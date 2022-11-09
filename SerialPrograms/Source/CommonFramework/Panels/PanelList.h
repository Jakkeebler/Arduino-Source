/*  Panel List
 *
 *  From: https://github.com/PokemonAutomation/Arduino-Source
 *
 */

#ifndef PokemonAutomation_PanelList_H
#define PokemonAutomation_PanelList_H

#include <vector>
#include "CommonFramework/Panels/PanelTools.h"

class QWidget;

namespace PokemonAutomation{

class PanelListWidget;


class PanelListDescriptor{
    using MakePanelEntries = std::vector<PanelEntry>(*)();

public:
    virtual ~PanelListDescriptor() = default;
    PanelListDescriptor(std::string name, bool enabled = true);

    const std::string& name() const{ return m_name; }
    bool enabled() const{ return m_enabled; }

    virtual std::vector<PanelEntry> make_panels() const = 0;

public:
    PanelListWidget* make_QWidget(QWidget& parent, PanelHolder& holder) const;

protected:
    std::string m_name;
    bool m_enabled;
};



}
#endif
