import QtQuick
import org.kde.plasma.plasmoid
import org.underthec 1.0

WallpaperItem {
    id: root

    readonly property var cfg: root.configuration

    function buildOptions() {
        var o = {};
        if (cfg.Classic > 0) {
            o["classic"] = cfg.Classic === 2 ? "1.1" : "1.0";
        } else {
            o["aquatic-life"] = "fish=" + (cfg.FishAuto ? "auto" : cfg.Fish) + cfg.Creatures.map(c => "," + c).join("");
        }
        o["message"] = cfg.Message;
        o["message-color"] = cfg.MessageColor;
        o["message-position"] = cfg.MessagePosition;
        o["castle-name"] = cfg.CastleName;
        o["no-castle"] = cfg.CastleEnabled ? "0" : "1";
        o["pace"] = cfg.Pace.toFixed(2);
        o["uturn-chance"] = String(cfg.UturnChance);
        o["fps"] = String(cfg.Fps);
        o["colors"] = cfg.Colors;
        return o;
    }

    UnderTheC {
        anchors.fill: parent
        options: root.buildOptions()
        fontSize: cfg.FontSize
    }
}
