#include <android/log.h>
#include <jni.h>

#include <stdint.h>
#include <string.h>

#include "../../app.h"
#include "../../config.h"
#include "../../xalloc.h"
#include "../../entity/priv.h"

struct handle {
  struct app app;
  jint *packed;
  size_t packed_cap;
};

static struct handle *from_ptr(jlong ptr) { return (struct handle *)(intptr_t)ptr; }

static char *dup_bytes(JNIEnv *env, jbyteArray arr) {
  jsize n = (*env)->GetArrayLength(env, arr);
  char *s = xmalloc((size_t)n + 1);
  (*env)->GetByteArrayRegion(env, arr, 0, n, (jbyte *)s);
  s[n] = '\0';
  return s;
}

static void throw_illegal(JNIEnv *env, const char *msg) {
  __android_log_print(ANDROID_LOG_ERROR, "underthec", "%s", msg);
  jclass cls = (*env)->FindClass(env, "java/lang/IllegalArgumentException");
  if (cls != NULL) (*env)->ThrowNew(env, cls, msg);
}

/* opts: name0, value0, name1, value1... as utf-8 bytes */
JNIEXPORT jlong JNICALL Java_org_underthec_Native_create(JNIEnv *env, jclass cls, jobjectArray opts, jdouble now) {
  (void)cls;
  struct config cfg;
  char err[256];
  config_init(&cfg);
  jsize n = (*env)->GetArrayLength(env, opts);
  for (jsize i = 0; i + 1 < n; i += 2) {
    jbyteArray jn = (jbyteArray)(*env)->GetObjectArrayElement(env, opts, i);
    jbyteArray jv = (jbyteArray)(*env)->GetObjectArrayElement(env, opts, i + 1);
    char *name = dup_bytes(env, jn);
    char *value = dup_bytes(env, jv);
    (*env)->DeleteLocalRef(env, jn);
    (*env)->DeleteLocalRef(env, jv);
    bool ok = config_set(&cfg, name, value, name, err, sizeof err);
    free(name);
    free(value);
    if (!ok) {
      config_free(&cfg);
      throw_illegal(env, err);
      return 0;
    }
  }
  if (!config_check(&cfg, err, sizeof err)) {
    config_free(&cfg);
    throw_illegal(env, err);
    return 0;
  }
  struct handle *h = xcalloc(1, sizeof *h);
  config_start(&cfg, &h->app, now);
  config_free(&cfg);
  return (jlong)(intptr_t)h;
}

JNIEXPORT void JNICALL Java_org_underthec_Native_destroy(JNIEnv *env, jclass cls, jlong ptr) {
  (void)env;
  (void)cls;
  struct handle *h = from_ptr(ptr);
  if (h == NULL) return;
  app_free(&h->app);
  free(h->packed);
  free(h);
}

static void reserve_packed(struct handle *h, size_t n) {
  if (n <= h->packed_cap) return;
  h->packed = xrealloc(h->packed, n * sizeof(*h->packed));
  h->packed_cap = n;
}

JNIEXPORT void JNICALL Java_org_underthec_Native_resize(JNIEnv *env, jclass cls, jlong ptr, jint cols, jint rows) {
  (void)env;
  (void)cls;
  struct handle *h = from_ptr(ptr);
  app_resize(&h->app, cols, rows);
  reserve_packed(h, (size_t)h->app.canvas.width * (size_t)h->app.canvas.height);
}

JNIEXPORT void JNICALL Java_org_underthec_Native_key(JNIEnv *env, jclass cls, jlong ptr, jint key) {
  (void)env;
  (void)cls;
  app_key(&from_ptr(ptr)->app, key);
}

JNIEXPORT void JNICALL Java_org_underthec_Native_click(JNIEnv *env, jclass cls, jlong ptr, jint col, jint row) {
  (void)env;
  (void)cls;
  app_click(&from_ptr(ptr)->app, col, row);
}

JNIEXPORT void JNICALL Java_org_underthec_Native_frame(JNIEnv *env, jclass cls, jlong ptr, jdouble now, jintArray out) {
  (void)cls;
  struct handle *h = from_ptr(ptr);
  app_frame(&h->app, now);
  size_t n = (size_t)h->app.canvas.width * (size_t)h->app.canvas.height;
  for (size_t i = 0; i < n; i++) {
    const struct cell *c = &h->app.canvas.cells[i];
    uint32_t cp = c->cont ? 0 : (uint32_t)utf8_decode(c->glyph, utf8_seq_len((unsigned char)c->glyph[0]));
    h->packed[i] = (jint)(cp << 10 | (c->bg_bold ? 1u : 0u) << 9 | (uint32_t)c->bg << 5 | (uint32_t)c->col << 1 | (c->bold ? 1u : 0u));
  }
  jsize len = (*env)->GetArrayLength(env, out);
  (*env)->SetIntArrayRegion(env, out, 0, (jsize)n < len ? (jsize)n : len, h->packed);
}

JNIEXPORT jint JNICALL Java_org_underthec_Native_width(JNIEnv *env, jclass cls, jlong ptr) {
  (void)env;
  (void)cls;
  return from_ptr(ptr)->app.canvas.width;
}

JNIEXPORT jint JNICALL Java_org_underthec_Native_height(JNIEnv *env, jclass cls, jlong ptr) {
  (void)env;
  (void)cls;
  return from_ptr(ptr)->app.canvas.height;
}

JNIEXPORT jint JNICALL Java_org_underthec_Native_fps(JNIEnv *env, jclass cls, jlong ptr) {
  (void)env;
  (void)cls;
  return from_ptr(ptr)->app.fps;
}

JNIEXPORT jboolean JNICALL Java_org_underthec_Native_dialogOpen(JNIEnv *env, jclass cls, jlong ptr) {
  (void)env;
  (void)cls;
  const struct app *a = &from_ptr(ptr)->app;
  return settings_ui_is_open(&a->settings) || help_ui_is_open(&a->help);
}
