/*  Static Text
 *
 *  From: https://github.com/PokemonAutomation/Arduino-Source
 *
 */

#ifndef PokemonAutomation_StaticTextOption_H
#define PokemonAutomation_StaticTextOption_H

#include "Common/Qt/Options/ConfigOption.h"

namespace PokemonAutomation{



class StaticTextOption : public ConfigOption{
public:
    StaticTextOption(std::string label);

    const std::string& label() const{ return m_label; }

    virtual void load_json(const JsonValue& json) override;
    virtual JsonValue to_json() const override;

    virtual ConfigWidget* make_ui(QWidget& parent) override;

private:
    friend class StaticTextWidget;
    std::string m_label;
};



class SectionDividerOption : public ConfigOption{
public:
    SectionDividerOption(std::string label);

    void set_label(std::string label);

    virtual void load_json(const JsonValue& json) override;
    virtual JsonValue to_json() const override;

    virtual ConfigWidget* make_ui(QWidget& parent) override;

private:
    friend class SectionDividerWidget;
    std::string m_label;
};





}
#endif


