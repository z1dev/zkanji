#ifndef ZDICTIONARYCOMBOBOX_H
#define ZDICTIONARYCOMBOBOX_H


#include <QComboBox>

class ZDictionaryComboBox : public QComboBox
{
    Q_OBJECT
public:
    ZDictionaryComboBox(QWidget *parent = nullptr);
    virtual ~ZDictionaryComboBox();
private:
    typedef QComboBox   base;
};


#endif

