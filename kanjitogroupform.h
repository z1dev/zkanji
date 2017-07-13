#ifndef KANJITOGROUPFORM_H
#define KANJITOGROUPFORM_H

#include <QComboBox>
#include <QPushButton>
#include "dialogwindow.h"

namespace Ui {
    class KanjiToGroupForm;
}

class GroupBase;
class Dictionary;
class KanjiListModel;
class KanjiToGroupForm : public DialogWindow
{
    Q_OBJECT
public:
    KanjiToGroupForm(QWidget *parent = nullptr);
    virtual ~KanjiToGroupForm();

    void exec(Dictionary *d, ushort kindex, GroupBase *initialSelection = nullptr);
    void exec(Dictionary *d, const std::vector<ushort> kindexes, GroupBase *initialSelection = nullptr);
protected:
    virtual void closeEvent(QCloseEvent *e) override;
private slots:
    // Click slot for the button for adding kanji to a group.
    void accept();
    // Items have been selected or deselected in the group widget.
    void selChanged();
    // Changes the dictionary displayed in the group widget.
    void setDictionary(int index);

    void dictionaryToBeRemoved(int index, int orderindex, Dictionary *dict);
private:
    Ui::KanjiToGroupForm *ui;

    Dictionary *dict;
    KanjiListModel *model;

    QComboBox *dictCBox;
    QPushButton *acceptButton;

    typedef DialogWindow    base;
};

#endif // KANJITOGROUPFORM_H
