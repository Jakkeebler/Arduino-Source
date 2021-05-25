/*  Persistent Settings
 *
 *  From: https://github.com/PokemonAutomation/Arduino-Source
 *
 */

#include <QCoreApplication>
#include <QFile>
#include <QJsonValue>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>
#include "Common/Qt/StringException.h"
#include "Common/Qt/QtJsonTools.h"
#include "NintendoSwitch/Framework/VirtualSwitchControllerMapping.h"
#include "PanelList.h"
#include "PersistentSettings.h"

#include <iostream>
using std::cout;
using std::endl;

namespace PokemonAutomation{




PersistentSettings& PERSISTENT_SETTINGS(){
    static PersistentSettings settings;
    return settings;
}


PersistentSettings::PersistentSettings()
    : stats_file("PA-Stats.txt")
    , window_size(960, 540)
    , naughty_mode(false)
    , developer_mode(false)
    , log_everything(false)
{
    //  Find the resource directory.
    QString path = QCoreApplication::applicationDirPath();
    for (size_t c = 0; c < 5; c++){
        QFile file(path + "/Resources/");
//        cout << path.toUtf8().data() << endl;
        if (file.exists()){
            resource_path = path + "/Resources/";
            break;
        }
        path += "/..";
    }
    if (resource_path.isEmpty()){
        resource_path = QCoreApplication::applicationDirPath() + "/../Resources/";
    }
//    cout << "resources = " << resource_path.toUtf8().data() << endl;
}



void PersistentSettings::write() const{
    QJsonObject root;
//    root.insert("00-ConfigPath", QJsonValue(config_path));
//    root.insert("01-SourcePath", QJsonValue(source_path));
    root.insert("01-StatsFile", QJsonValue(stats_file));
    {
        QJsonArray res;
        res.append(QJsonValue(window_size.width()));
        res.append(QJsonValue(window_size.height()));
        root.insert("02-WindowSize", res);
    }
    root.insert("03-NaughtyMode", QJsonValue(naughty_mode));
    root.insert("04-DeveloperMode", QJsonValue(developer_mode));
    root.insert("05-LogEverything", QJsonValue(log_everything.load(std::memory_order_acquire)));
//    root.insert("06-ResourcePath", QJsonValue(resource_path));

    root.insert("10-SwitchKeyboardMapping", NintendoSwitch::read_keyboard_mapping());

    QJsonObject settings;
    for (const auto& panel : SETTINGS_MAP()){
        settings.insert(panel.first, panel.second->to_json());
    }
    root.insert("98-SharedSettings", settings);

    QJsonObject programs;
    for (const auto& panel : PROGRAM_MAP()){
        programs.insert(panel.first, panel.second->to_json());
    }
    root.insert("99-ProgramSettings", programs);

    write_json_file(QCoreApplication::applicationFilePath() + "-Settings.json", QJsonDocument(root));
}
void PersistentSettings::read(){
    QJsonDocument doc = read_json_file(QCoreApplication::applicationFilePath() + "-Settings.json");
    if (!doc.isObject()){
        throw StringException("Invalid settings file.");
    }
    QJsonObject root = doc.object();
//    json_get_string(config_path, root, "00-ConfigPath");
//    json_get_string(source_path, root, "01-SourcePath");
    json_get_string(stats_file, root, "01-StatsFile");
//    stat_sets.open_from_file(stats_file);

    {
        QJsonArray res = json_get_array_nothrow(root, "02-WindowSize");
        if (res.size() == 2){
            window_size = QSize(
                res[0].toInt(window_size.width()),
                res[1].toInt(window_size.height())
            );
        }
    }
    json_get_bool(naughty_mode, root, "03-NaughtyMode");
    json_get_bool(developer_mode, root, "04-DeveloperMode");
    json_get_bool(log_everything, root, "05-LogEverything");
//    json_get_string(resource_path, root, "06-ResourcePath");
    NintendoSwitch::set_keyboard_mapping(json_get_array_nothrow(root, "10-SwitchKeyboardMapping"));
    settings = json_get_object_nothrow(root, "98-SharedSettings");
    programs = json_get_object_nothrow(root, "99-ProgramSettings");
}



}

