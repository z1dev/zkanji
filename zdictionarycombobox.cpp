#include "zdictionarycombobox.h"
#include "zdictionariesmodel.h"
#include "generalsettings.h"

ZDictionaryComboBox::ZDictionaryComboBox(QWidget *parent) : base(parent)
{
    setModel(ZKanji::dictionariesModel());
    setIconSize(Settings::scaled(iconSize()));
}

ZDictionaryComboBox::~ZDictionaryComboBox()
{
}

