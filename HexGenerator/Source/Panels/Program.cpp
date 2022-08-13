/*  Parent Class for all Programs
 *
 *  From: https://github.com/PokemonAutomation/Arduino-Source
 *
 */

#include <iostream>
using std::cout;
using std::endl;

#include <QDir>
#include <QFile>
#include <QLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QMessageBox>
#include "Common/Cpp/Exceptions.h"
#include "Common/Cpp/Json/JsonArray.h"
#include "Common/Cpp/Json/JsonObject.h"
#include "Tools/Tools.h"
#include "Tools/PersistentSettings.h"
#include "UI/MainWindow.h"
#include "Program.h"

namespace PokemonAutomation{
namespace HexGenerator{


const std::string Program::JSON_PROGRAM_NAME    = "0-ProgramName";
const std::string Program::JSON_DOCUMENTATION   = "1-Documentation";
const std::string Program::JSON_DESCRIPTION     = "2-Description";
const std::string Program::JSON_PARAMETERS      = "3-Parameters";


const std::string Program::BUILD_BUTTON_NORMAL  = "Save and generate .hex file!";
const std::string Program::BUILD_BUTTON_BUSY    = "Build in progress... Please Wait.";

Program::Program(std::string category, std::string name, std::string description, std::string doc_link)
    : RightPanel(
        std::move(category),
        std::move(name),
        std::move(doc_link)
    )
    , m_description(std::move(description))
{}
Program::Program(std::string category, const JsonObject& obj)
    : RightPanel(
        std::move(category),
        obj.get_string_throw(JSON_PROGRAM_NAME),
        obj.get_string_throw(JSON_DOCUMENTATION)
    )
    , m_description(obj.get_string_throw(JSON_DESCRIPTION))
{}
Program::~Program(){
    if (m_builder.joinable()){
        m_builder.join();
    }
}

QWidget* Program::make_options_body(QWidget& parent){
    QWidget* box = new QWidget(&parent);
    QVBoxLayout* layout = new QVBoxLayout(box);
    QLabel* label = new QLabel("There are no program-specific options for this program.");
    label->setAlignment(Qt::AlignCenter);
    layout->addWidget(label);
    return box;
}

JsonArray Program::parameters_json() const{
    return JsonArray();
}

JsonObject Program::to_json() const{
    JsonObject root;
    root[JSON_PROGRAM_NAME] = m_name;
    root[JSON_DOCUMENTATION] = m_doc_link;
    root[JSON_DESCRIPTION] = m_description;
    root[JSON_PARAMETERS] = parameters_json();
    return root;
}
std::string Program::to_cfile() const{
    std::string body;
    body += "//  This file is generated by the UI. There's no point in editing.\r\n";
    body += "#include \"";
    body += "../NativePrograms/";
    body += m_category;
    body += "/Programs/";
    body += m_name;
    body += ".h\"\r\n";
    body += parameters_cpp();
    return body;
}
std::string Program::save_json() const{
//    cout << QCoreApplication::applicationDirPath().toUtf8().data() << endl;
    std::string name = settings.path + CONFIG_FOLDER_NAME + "/" + m_category + "/" + m_name + ".json";
    to_json().dump(name);
    return name;
}
std::string Program::save_cfile() const{
    std::string name = settings.path + SOURCE_FOLDER_NAME + "/" + m_category + "/" + m_name + ".c";
    std::string cpp = to_cfile();
    QFile file(QString::fromStdString(name));
    if (!file.open(QFile::WriteOnly)){
        throw FileException(nullptr, PA_CURRENT_FUNCTION, "Unable to create source file.", name);
    }
    if (file.write(cpp.c_str(), cpp.size()) != cpp.size()){
        throw FileException(nullptr, PA_CURRENT_FUNCTION, "Unable to write source file.", name);
    }
    file.close();
    return name;
}

class ButtonRestore{
public:
    ButtonRestore(QPushButton* button)
        : m_button(button)
    {}
    ~ButtonRestore(){
        QPushButton* button = m_button;
        run_on_main_thread([=]{
            button->setText(QString::fromStdString(Program::BUILD_BUTTON_NORMAL));
        });
    }
private:
    QPushButton* m_button;
};

void Program::save_and_build(const std::string& board){
//        cout << "asdf" << endl;
    std::string error = check_validity();
    if (!error.empty()){
        QMessageBox box;
        box.critical(nullptr, "Error", QString::fromStdString(error));
        return;
    }
//    if (!QDir(settings.config_path).exists()){
//        QMessageBox box;
//        box.critical(nullptr, "Error", "Source directory not found. Please unzip the package if you haven't already.");
//        return;
//    }

    if (m_builder.joinable()){
        m_builder.join();
    }

    m_build_button->setText(QString::fromStdString(BUILD_BUTTON_BUSY));
    m_builder = std::thread([=]{
        ButtonRestore restore(m_build_button);
        try{
            save_json();
            save_cfile();
        }catch (const Exception& e){
            QString error = QString::fromStdString(e.message());
            run_on_main_thread([=]{
                QMessageBox box;
                box.critical(nullptr, "Error", error);
            });
            return;
        }

        //  Build
        std::string hex_file = settings.path + m_category + "-" + m_name + ("-" + board + ".hex").c_str();
        std::string log_file = settings.path + LOG_FOLDER_NAME + "/" + m_name + ("-" + board).c_str() + ".log";
        if (build_hexfile(board, m_category, m_name, hex_file, log_file) != 0){
            return;
        }

        //  Report results.
        run_on_main_thread([=]{
            if (QFileInfo(QString::fromStdString(hex_file)).exists()){
                QMessageBox box;
                box.information(nullptr, "Success!", QString::fromStdString(".hex file has been built!\r\n" + hex_file));
            }else{
                QMessageBox box;
                box.critical(nullptr, "Error", QString::fromStdString(".hex was not built. Please check error log.\r\n" + log_file));
            }
        });
    });
}

QWidget* Program::make_ui(MainWindow& parent){
    QWidget* box = new QWidget(&parent);
    QVBoxLayout* layout = new QVBoxLayout(box);
    layout->setMargin(0);
//    layout->setAlignment(Qt::AlignTop);
    layout->addWidget(new QLabel(QString::fromStdString("<font size=4><b>Name:</b></font> " + m_name)));
    {
        QLabel* text = new QLabel(QString::fromStdString("<font size=4><b>Description:</b></font> " + m_description));
        layout->addWidget(text);
        text->setWordWrap(true);
    }
    {
        std::string path = GITHUB_REPO + m_doc_link;
        QLabel* text = new QLabel(QString::fromStdString("<font size=4><a href=\"" + path + "\">Online Documentation for " + m_name + "</a></font>"));
        layout->addWidget(text);
        text->setTextFormat(Qt::RichText);
        text->setTextInteractionFlags(Qt::TextBrowserInteraction);
        text->setOpenExternalLinks(true);
    }
    layout->addWidget(make_options_body(*box), Qt::AlignBottom);

    QHBoxLayout* row = new QHBoxLayout();
    layout->addLayout(row);
    {
        QPushButton* button = new QPushButton("Save Settings", box);
        connect(
            button, &QPushButton::clicked,
            this, [&](bool){
                try{
                    std::string name = save_json();
                    QMessageBox box;
                    box.information(nullptr, "Success!", QString::fromStdString("Settings saved to: " + name + "\n"));
                }catch (const char* str){
                    QMessageBox box;
                    box.critical(nullptr, "Error", str);
                    return;
                }catch (const QString& str){
                    QMessageBox box;
                    box.critical(nullptr, "Error", str);
                    return;
                }
            }
        );
        row->addWidget(button);
    }
    {
        QPushButton* button = new QPushButton("Restore Defaults", box);
        connect(
            button, &QPushButton::clicked,
            this, [&](bool){
                restore_defaults();
                parent.change_panel(*this);
            }
        );
        row->addWidget(button);
    }
    {
        m_build_button = new QPushButton(QString::fromStdString(BUILD_BUTTON_NORMAL), box);
        QFont font = m_build_button->font();
        font.setPointSize(16);
        m_build_button->setFont(font);
        connect(
            m_build_button, &QPushButton::clicked,
            this, [&](bool){
                save_and_build(parent.current_board());
            }
        );
        layout->addWidget(m_build_button);
    }
    return box;
}


}
}
