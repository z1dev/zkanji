/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#include <QStringBuilder>
#include "zstrings.h"
#include "grammar_enums.h"
#include "languages.h"

#include "checked_cast.h"

//-------------------------------------------------------------

#if 0
namespace
{
    // QMessageBox translations that should be not used directly.

    const char *ignore[] = {
        QT_TRANSLATE_NOOP("QPlatformTheme", "OK"), QT_TRANSLATE_NOOP("QPlatformTheme", "Cancel"), QT_TRANSLATE_NOOP("QPlatformTheme", "&Yes"), QT_TRANSLATE_NOOP("QPlatformTheme", "&No")

        QT_TRANSLATE_NOOP("QColorDialog", "Hu&e:"),
        QT_TRANSLATE_NOOP("QColorDialog", "&Sat:"),
        QT_TRANSLATE_NOOP("QColorDialog", "&Val:"),
        QT_TRANSLATE_NOOP("QColorDialog", "&Red:"),
        QT_TRANSLATE_NOOP("QColorDialog", "&Green:"),
        QT_TRANSLATE_NOOP("QColorDialog", "Bl&ue:"),
        QT_TRANSLATE_NOOP("QColorDialog", "A&lpha channel:"),
        QT_TRANSLATE_NOOP("QColorDialog", "&HTML:"),
        QT_TRANSLATE_NOOP("QColorDialog", "Cursor at %1, %2\nPress ESC to cancel"),
        QT_TRANSLATE_NOOP("QColorDialog", "Select Color"),
        QT_TRANSLATE_NOOP("QColorDialog", "&Basic colors"),
        QT_TRANSLATE_NOOP("QColorDialog", "&Custom colors"),
        QT_TRANSLATE_NOOP("QColorDialog", "&Add to Custom Colors"),
        QT_TRANSLATE_NOOP("QColorDialog", "&Pick Screen Color"),
    };
}
#endif

namespace Strings
{
    // Returns priority text like "High" fitting the dif difficulty value between 0 (lowest) to 8 (highest).
    QString priorities(int level)
    {
        static const char *texts[9] = {
            QT_TRANSLATE_NOOP("WordStudyListForm", "Lowest"), QT_TRANSLATE_NOOP("WordStudyListForm", "Very low"), QT_TRANSLATE_NOOP("WordStudyListForm", "Low"),
            QT_TRANSLATE_NOOP("WordStudyListForm", "Lower"), QT_TRANSLATE_NOOP("WordStudyListForm", "Normal"), QT_TRANSLATE_NOOP("WordStudyListForm", "Higher"),
            QT_TRANSLATE_NOOP("WordStudyListForm", "High"), QT_TRANSLATE_NOOP("WordStudyListForm", "Very high"), QT_TRANSLATE_NOOP("WordStudyListForm", "Highest")
        };

        return zLang->translate("WordStudyListForm", texts[level]);
    }


    QString capitalize(const QString &str)
    {
        return str[0].toUpper() % str.mid(1);

    }

    QString wordType(uchar type)
    {
        static const char *texts[(int)WordTypes::Count] = {
            QT_TRANSLATE_NOOP("Grammar", "n"),       QT_TRANSLATE_NOOP("Grammar", "-ru"),      QT_TRANSLATE_NOOP("Grammar", "-u"),           QT_TRANSLATE_NOOP("Grammar", "suru"),
            QT_TRANSLATE_NOOP("Grammar", "-suru"),    QT_TRANSLATE_NOOP("Grammar", "-aru"),     QT_TRANSLATE_NOOP("Grammar", "-kuru"),        QT_TRANSLATE_NOOP("Grammar", "-iku"),
            QT_TRANSLATE_NOOP("Grammar", "-ri"),      QT_TRANSLATE_NOOP("Grammar", "-zuru"),    QT_TRANSLATE_NOOP("Grammar", "tr"),          QT_TRANSLATE_NOOP("Grammar", "intr"),
            QT_TRANSLATE_NOOP("Grammar", "a"),       QT_TRANSLATE_NOOP("Grammar", "na"),       QT_TRANSLATE_NOOP("Grammar", "-to"),          QT_TRANSLATE_NOOP("Grammar", "pn-a"),
            QT_TRANSLATE_NOOP("Grammar", "no"),       QT_TRANSLATE_NOOP("Grammar", "adv"),     QT_TRANSLATE_NOOP("Grammar", "aux"),         QT_TRANSLATE_NOOP("Grammar", "pref"),
            QT_TRANSLATE_NOOP("Grammar", "suf"),     QT_TRANSLATE_NOOP("Grammar", "conj"),    QT_TRANSLATE_NOOP("Grammar", "int"),         QT_TRANSLATE_NOOP("Grammar", "exp"),
            QT_TRANSLATE_NOOP("Grammar", "pn"),    QT_TRANSLATE_NOOP("Grammar", "par"),     QT_TRANSLATE_NOOP("Grammar", "cnt"),         QT_TRANSLATE_NOOP("Grammar", "da/desu"),
            QT_TRANSLATE_NOOP("Grammar", "arch v"),  QT_TRANSLATE_NOOP("Grammar", "arch a"),  QT_TRANSLATE_NOOP("Grammar", "arch -na")
        };

        return zLang->translate("Grammar", texts[type]);
    }

    QString wordTypeLong(uchar type)
    {
        static const char *texts[(int)WordTypes::Count] = {
            QT_TRANSLATE_NOOP("Grammar", "noun"), QT_TRANSLATE_NOOP("Grammar", "ichidan verb"), QT_TRANSLATE_NOOP("Grammar", "godan verb"), QT_TRANSLATE_NOOP("Grammar", "verb with suru"),
            QT_TRANSLATE_NOOP("Grammar", "-suru verb"), QT_TRANSLATE_NOOP("Grammar", "-aru verb"), QT_TRANSLATE_NOOP("Grammar", "-kuru verb"), QT_TRANSLATE_NOOP("Grammar", "-iku verb"),
            QT_TRANSLATE_NOOP("Grammar", "-ri verb"), QT_TRANSLATE_NOOP("Grammar", "-zuru verb"), QT_TRANSLATE_NOOP("Grammar", "transitive verb"), QT_TRANSLATE_NOOP("Grammar", "intransitive verb"),
            QT_TRANSLATE_NOOP("Grammar", "true adjective (-i)"), QT_TRANSLATE_NOOP("Grammar", "adjectival noun (-na)"), QT_TRANSLATE_NOOP("Grammar", "taru adjective (-to)"), QT_TRANSLATE_NOOP("Grammar", "pre-noun adjective"),
            QT_TRANSLATE_NOOP("Grammar", "may take no"), QT_TRANSLATE_NOOP("Grammar", "adverb"), QT_TRANSLATE_NOOP("Grammar", "auxiliary"), QT_TRANSLATE_NOOP("Grammar", "prefix"),
            QT_TRANSLATE_NOOP("Grammar", "suffix"), QT_TRANSLATE_NOOP("Grammar", "conjunction"), QT_TRANSLATE_NOOP("Grammar", "interjection"), QT_TRANSLATE_NOOP("Grammar", "expression"),
            QT_TRANSLATE_NOOP("Grammar", "pronoun"), QT_TRANSLATE_NOOP("Grammar", "particle"), QT_TRANSLATE_NOOP("Grammar", "counter"), QT_TRANSLATE_NOOP("Grammar", "copula da"),
            QT_TRANSLATE_NOOP("Grammar", "archaic verb"), QT_TRANSLATE_NOOP("Grammar", "archaic adjective"), QT_TRANSLATE_NOOP("Grammar", "archaic adjectival noun")
        };

        return zLang->translate("Grammar", texts[type]);
    }

    QString wordNote(uchar note)
    {
        static const char *texts[(int)WordNotes::Count] = {
            QT_TRANSLATE_NOOP("Grammar", "kana"),     QT_TRANSLATE_NOOP("Grammar", "abbr"),    QT_TRANSLATE_NOOP("Grammar", "4k"),   QT_TRANSLATE_NOOP("Grammar", "idiom"),
            QT_TRANSLATE_NOOP("Grammar", "obsc"),    QT_TRANSLATE_NOOP("Grammar", "obso"),    QT_TRANSLATE_NOOP("Grammar", "onom"),    QT_TRANSLATE_NOOP("Grammar", "pv"),
            QT_TRANSLATE_NOOP("Grammar", "rare"),     QT_TRANSLATE_NOOP("Grammar", "sens"),    QT_TRANSLATE_NOOP("Grammar", "col"),     QT_TRANSLATE_NOOP("Grammar", "fam"),
            QT_TRANSLATE_NOOP("Grammar", "hon"),     QT_TRANSLATE_NOOP("Grammar", "hum"),     QT_TRANSLATE_NOOP("Grammar", "pol"),     QT_TRANSLATE_NOOP("Grammar", "arch"),
            QT_TRANSLATE_NOOP("Grammar", "poet"),    QT_TRANSLATE_NOOP("Grammar", "chl"),     QT_TRANSLATE_NOOP("Grammar", "male"),       QT_TRANSLATE_NOOP("Grammar", "fem"),
            QT_TRANSLATE_NOOP("Grammar", "joke"),     QT_TRANSLATE_NOOP("Grammar", "sl"),      QT_TRANSLATE_NOOP("Grammar", "msl"),     QT_TRANSLATE_NOOP("Grammar", "der"),
            QT_TRANSLATE_NOOP("Grammar", "vul"),
        };

        return zLang->translate("Grammar", texts[note]);
    }

    QString wordNoteLong(uchar note)
    {
        static const char *texts[(int)WordNotes::Count] = {
            QT_TRANSLATE_NOOP("Grammar", "kana usage (usually no kanji)"), QT_TRANSLATE_NOOP("Grammar", "abbreviation"), QT_TRANSLATE_NOOP("Grammar", "four kanji idiom"), QT_TRANSLATE_NOOP("Grammar", "idiomatism"),
            QT_TRANSLATE_NOOP("Grammar", "obscure usage"), QT_TRANSLATE_NOOP("Grammar", "obsolete term"), QT_TRANSLATE_NOOP("Grammar", "onomatopoeia"), QT_TRANSLATE_NOOP("Grammar", "proverb"),
            QT_TRANSLATE_NOOP("Grammar", "rare word or phrase"), QT_TRANSLATE_NOOP("Grammar", "sensitivity"), QT_TRANSLATE_NOOP("Grammar", "colloquialism"), QT_TRANSLATE_NOOP("Grammar", "familiar language"),
            QT_TRANSLATE_NOOP("Grammar", "honorific language"), QT_TRANSLATE_NOOP("Grammar", "humble language"), QT_TRANSLATE_NOOP("Grammar", "polite language"), QT_TRANSLATE_NOOP("Grammar", "archaic expression"),
            QT_TRANSLATE_NOOP("Grammar", "poetic expression"), QT_TRANSLATE_NOOP("Grammar", "children's language"), QT_TRANSLATE_NOOP("Grammar", "male language"), QT_TRANSLATE_NOOP("Grammar", "female language"),
            QT_TRANSLATE_NOOP("Grammar", "joking style"), QT_TRANSLATE_NOOP("Grammar", "slang"), QT_TRANSLATE_NOOP("Grammar", "manga slang"), QT_TRANSLATE_NOOP("Grammar", "derogatory expression"),
            QT_TRANSLATE_NOOP("Grammar", "vulgar expression"),
        };

        return zLang->translate("Grammar", texts[note]);
    }

    QString wordField(uchar field)
    {
        static const char *texts[(int)WordFields::Count] = {
            QT_TRANSLATE_NOOP("Grammar", "archi"), QT_TRANSLATE_NOOP("Grammar", "bus"), QT_TRANSLATE_NOOP("Grammar", "comp"), QT_TRANSLATE_NOOP("Grammar", "econ"),
            QT_TRANSLATE_NOOP("Grammar", "engi"), QT_TRANSLATE_NOOP("Grammar", "fin"), QT_TRANSLATE_NOOP("Grammar", "food"), QT_TRANSLATE_NOOP("Grammar", "law"),
            QT_TRANSLATE_NOOP("Grammar", "mil"), QT_TRANSLATE_NOOP("Grammar", "mus"),

            QT_TRANSLATE_NOOP("Grammar", "anat"), QT_TRANSLATE_NOOP("Grammar", "astr"), QT_TRANSLATE_NOOP("Grammar", "biol"), QT_TRANSLATE_NOOP("Grammar", "bot"),
            QT_TRANSLATE_NOOP("Grammar", "chem"), QT_TRANSLATE_NOOP("Grammar", "geol"), QT_TRANSLATE_NOOP("Grammar", "geom"), QT_TRANSLATE_NOOP("Grammar", "ling"),
            QT_TRANSLATE_NOOP("Grammar", "math"), QT_TRANSLATE_NOOP("Grammar", "med"), QT_TRANSLATE_NOOP("Grammar", "phys"), QT_TRANSLATE_NOOP("Grammar", "zoo"),

            QT_TRANSLATE_NOOP("Grammar", "Buddh"), QT_TRANSLATE_NOOP("Grammar", "Shinto"),

            QT_TRANSLATE_NOOP("Grammar", "baseb"), QT_TRANSLATE_NOOP("Grammar", "mahj"), QT_TRANSLATE_NOOP("Grammar", "m.a."), QT_TRANSLATE_NOOP("Grammar", "shogi"),
            QT_TRANSLATE_NOOP("Grammar", "sports"), QT_TRANSLATE_NOOP("Grammar", "sumo")
        };

        return zLang->translate("Grammar", texts[field]);
    }

    QString wordFieldLong(uchar field)
    {
        static const char *texts[(int)WordFields::Count] = {
            QT_TRANSLATE_NOOP("Grammar", "architecture"), QT_TRANSLATE_NOOP("Grammar", "business"), QT_TRANSLATE_NOOP("Grammar", "computing"), QT_TRANSLATE_NOOP("Grammar", "economy"),
            QT_TRANSLATE_NOOP("Grammar", "engineering"), QT_TRANSLATE_NOOP("Grammar", "finance"), QT_TRANSLATE_NOOP("Grammar", "food"), QT_TRANSLATE_NOOP("Grammar", "law"),
            QT_TRANSLATE_NOOP("Grammar", "military"), QT_TRANSLATE_NOOP("Grammar", "music"),

            QT_TRANSLATE_NOOP("Grammar", "anatomy"), QT_TRANSLATE_NOOP("Grammar", "astronomy"), QT_TRANSLATE_NOOP("Grammar", "biology"), QT_TRANSLATE_NOOP("Grammar", "botany"),
            QT_TRANSLATE_NOOP("Grammar", "chemistry"), QT_TRANSLATE_NOOP("Grammar", "geology"), QT_TRANSLATE_NOOP("Grammar", "geometry"), QT_TRANSLATE_NOOP("Grammar", "linguistics"),
            QT_TRANSLATE_NOOP("Grammar", "mathematics"), QT_TRANSLATE_NOOP("Grammar", "medicine"), QT_TRANSLATE_NOOP("Grammar", "physics"), QT_TRANSLATE_NOOP("Grammar", "zoology"),

            QT_TRANSLATE_NOOP("Grammar", "Buddhism"), QT_TRANSLATE_NOOP("Grammar", "Shinto"),

            QT_TRANSLATE_NOOP("Grammar", "baseball"), QT_TRANSLATE_NOOP("Grammar", "mahjong"), QT_TRANSLATE_NOOP("Grammar", "martial arts"), QT_TRANSLATE_NOOP("Grammar", "shogi"),
            QT_TRANSLATE_NOOP("Grammar", "sports"), QT_TRANSLATE_NOOP("Grammar", "sumo")
        };

        return zLang->translate("Grammar", texts[field]);
    }

    QString wordDialect(uchar dia)
    {
        static const char *texts[(int)WordDialects::Count] = {
            QT_TRANSLATE_NOOP("Grammar", "Ho"), QT_TRANSLATE_NOOP("Grammar", "Kans"), QT_TRANSLATE_NOOP("Grammar", "Kant"), QT_TRANSLATE_NOOP("Grammar", "Kyo"),
            QT_TRANSLATE_NOOP("Grammar", "Kyu"), QT_TRANSLATE_NOOP("Grammar", "Na"), QT_TRANSLATE_NOOP("Grammar", "Os"), QT_TRANSLATE_NOOP("Grammar", "Ry"),
            QT_TRANSLATE_NOOP("Grammar", "Tos"), QT_TRANSLATE_NOOP("Grammar", "Tou"), QT_TRANSLATE_NOOP("Grammar", "Ts"),
        };

        return zLang->translate("Grammar", texts[dia]);
    }

    QString wordDialectLong(uchar dia)
    {
        static const char *texts[(int)WordDialects::Count] = {
            QT_TRANSLATE_NOOP("Grammar", "Hokkaidou dialect"), QT_TRANSLATE_NOOP("Grammar", "Kansai dialect"), QT_TRANSLATE_NOOP("Grammar", "Kantou dialect"), QT_TRANSLATE_NOOP("Grammar", "Kyoto dialect"),
            QT_TRANSLATE_NOOP("Grammar", "Kyuushuu dialect"), QT_TRANSLATE_NOOP("Grammar", "Nagano dialect"), QT_TRANSLATE_NOOP("Grammar", "Osaka dialect"), QT_TRANSLATE_NOOP("Grammar", "Ryuukyuu dialect"),
            QT_TRANSLATE_NOOP("Grammar", "Tosa dialect"), QT_TRANSLATE_NOOP("Grammar", "Touhoku dialect"), QT_TRANSLATE_NOOP("Grammar", "Tsugaru dialect"),
        };

        return zLang->translate("Grammar", texts[dia]);
    }

    QString wordInfo(uchar inf)
    {
        static const char *texts[(int)WordInfo::Count] = {
            QT_TRANSLATE_NOOP("Grammar", "ate"), QT_TRANSLATE_NOOP("Grammar", "gik"), QT_TRANSLATE_NOOP("Grammar", "i.kanji"), QT_TRANSLATE_NOOP("Grammar", "i.kana"),
            QT_TRANSLATE_NOOP("Grammar", "i.oku"), QT_TRANSLATE_NOOP("Grammar", "o.kanji"), QT_TRANSLATE_NOOP("Grammar", "o.kana")
        };

        return zLang->translate("Grammar", texts[inf]);
    }

    QString wordInfoLong(uchar inf)
    {
        static const char *texts[(int)WordInfo::Count] = {
            QT_TRANSLATE_NOOP("Grammar", "ateji"), QT_TRANSLATE_NOOP("Grammar", "gikun"), QT_TRANSLATE_NOOP("Grammar", "irregular kanji"), QT_TRANSLATE_NOOP("Grammar", "irregular kana"),
            QT_TRANSLATE_NOOP("Grammar", "irregular okurigana"), QT_TRANSLATE_NOOP("Grammar", "outdated kanji"), QT_TRANSLATE_NOOP("Grammar", "outdated kana")
        };

        return zLang->translate("Grammar", texts[inf]);
    }

    // Short tags used for export/import.
    static const QString infotexts[(int)WordInfo::Count] = {
        "i_ate", "i_gik", "i_ikanji", "i_ikana", "i_ioku", "i_okanji", "i_okana"
    };
    QString wordInfoTags(uchar infs)
    {

        QString result;
        for (int ix = 0; infs != 0 && ix != (int)WordInfo::Count; ++ix)
            if ((infs & (1 << ix)) != 0)
                result += infotexts[ix] % ";";
        return result;
    }

    // Short tags used for export/import.
    static const QString typetexts[(int)WordTypes::Count] = {
        "t_n", "t_ru", "t_u", "t_suru",
        "t_suru2", "t_aru", "t_kuru", "t_iku",
        "t_ri", "t_zuru", "t_tr", "t_intr",
        "t_a", "t_na", "t_to", "t_pna",
        "t_no", "t_adv", "t_aux", "t_pref",
        "t_suf", "t_conj", "t_int", "t_exp",
        "t_pn", "t_par", "t_cnt", "t_da",
        "t_archv", "t_archa", "t_archna"
    };

    QString wordTypeTags(uint types)
    {
        QString result;
        for (int ix = 0; types != 0 && ix != (int)WordTypes::Count; ++ix)
            if ((types & (1 << ix)) != 0)
                result += typetexts[ix] % ";";
        return result;
    }

    // Short tags used for export/import.
    static const QString notetexts[(int)WordNotes::Count] = {
        "n_kana", "n_abbr", "n_fk", "n_idiom",
        "n_obsc", "n_obso", "n_onom", "n_pv",
        "n_rare", "n_sens", "n_col", "n_fam",
        "n_hon", "n_hum", "n_pol", "n_arch",
        "n_poet", "n_chl", "n_male", "n_fem",
        "n_joke", "n_sl", "n_msl", "n_der",
        "n_vul"
    };
    QString wordNoteTags(uint notes)
    {
        QString result;
        for (int ix = 0; notes != 0 && ix != (int)WordNotes::Count; ++ix)
            if ((notes & (1 << ix)) != 0)
                result += notetexts[ix] % ";";
        return result;
    }

    // Short tags used for export/import.
    static const QString fieldtexts[(int)WordFields::Count] = {
        "f_archi", "f_bus", "f_comp", "f_econ",
        "f_engi", "f_fin", "f_food", "f_law",
        "f_mil", "f_mus",
        "f_anat", "f_astr", "f_biol", "f_bot",
        "f_chem", "f_geol", "f_geom", "f_ling",
        "f_math", "f_med", "f_phys", "f_zoo",
        "f_buddh", "f_shinto",
        "f_baseb", "f_mahj", "f_ma", "f_shogi",
        "f_sports", "f_sumo"
    };
    QString wordFieldTags(uint fields)
    {
        QString result;
        for (int ix = 0; fields != 0 && ix != (int)WordFields::Count; ++ix)
            if ((fields & (1 << ix)) != 0)
                result += fieldtexts[ix] % ";";
        return result;
    }

    // Short tags used for export/import.
    static const QString dialecttexts[(int)WordDialects::Count] = {
        "d_ho", "d_kans", "d_kant", "d_kyo",
        "d_kyu", "d_na", "d_os", "d_ry",
        "d_tos", "d_tou", "d_ts",
    };
    QString wordDialectTags(ushort dials)
    {
        QString result;
        for (int ix = 0; dials != 0 && ix != (int)WordDialects::Count; ++ix)
            if ((dials & (1 << ix)) != 0)
                result += dialecttexts[ix] % ";";
        return result;
    }

    template<typename RET, int CNT>
    RET wordTagT(const QString texts[CNT], const QString &str, bool *ok)
    {
        if (ok != nullptr)
            *ok = false;

        RET result = 0;

        int pos = 0;
        int siz = str.size();
        while (pos != siz)
        {
            int p2 = pos;
            for (; pos != siz; ++pos)
            {
                if (str.at(pos) == ';')
                    break;
                if ((str.at(pos) < 'a' || str.at(pos) > 'z') && (str.at(pos) < '0' || str.at(pos) > '9') && str.at(pos) != '_')
                    return 0;
            }
            // Missing ; at end of tag or empty tag.
            if (pos == siz || p2 == pos)
                return 0;

            for (int ix = 0; ix != CNT; ++ix)
                if (texts[ix] == str.mid(p2, pos - p2))
                {
                    result |= (1 << ix);
                    break;
                }

            ++pos;
        }

        if (ok != nullptr)
            *ok = true;
        return result;
    }

    uchar wordTagInfo(const QString &str, bool *ok)
    {
        return wordTagT<uchar, (int)WordInfo::Count>(infotexts, str, ok);
    }

    uint wordTagTypes(const QString &str, bool *ok)
    {
        return wordTagT<uint, (int)WordTypes::Count>(typetexts, str, ok);
    }

    uint wordTagNotes(const QString &str, bool *ok)
    {
        return wordTagT<uint, (int)WordNotes::Count>(notetexts, str, ok);
    }

    uint wordTagFields(const QString &str, bool *ok)
    {
        return wordTagT<uint, (int)WordFields::Count>(fieldtexts, str, ok);
    }

    ushort wordTagDialects(const QString &str, bool *ok)
    {
        return wordTagT<ushort, (int)WordDialects::Count>(dialecttexts, str, ok);
    }


    QString wordJLPTLevel(uchar lv)
    {
        static const QString texts[5] = { "N5", "N4", "N3", "N2", "N1" };

        return texts[lv];
    }

    QString wordAttrib(uchar attr)
    {
        static const char *texts[(int)WordAttribs::Count] = {
            QT_TRANSLATE_NOOP("Grammar", "Word types"), QT_TRANSLATE_NOOP("Grammar", "Verb types"), QT_TRANSLATE_NOOP("Grammar", "Adjective types"), QT_TRANSLATE_NOOP("Grammar", "Archaic"),
            QT_TRANSLATE_NOOP("Grammar", "Word usage"), QT_TRANSLATE_NOOP("Grammar", "Relation of speaker and listener"), QT_TRANSLATE_NOOP("Grammar", "Style of speech"),
            QT_TRANSLATE_NOOP("Grammar", "Fields of use"), QT_TRANSLATE_NOOP("Grammar", "Sciences"), QT_TRANSLATE_NOOP("Grammar", "Religion"), QT_TRANSLATE_NOOP("Grammar", "Sports"),
            QT_TRANSLATE_NOOP("Grammar", "Dialects"), QT_TRANSLATE_NOOP("Grammar", "Character usage"), QT_TRANSLATE_NOOP("Grammar", "JLPT level")
        };

        return zLang->translate("Grammar", texts[attr]);
    }

    QString wordInflection(uchar infl)
    {
        static const char *winflectiontext[] = {
            QT_TRANSLATE_NOOP("Grammar", "polite"), QT_TRANSLATE_NOOP("Grammar", "polite past"), QT_TRANSLATE_NOOP("Grammar", "polite neg."), QT_TRANSLATE_NOOP("Grammar", "polite volitional / tentative"),
            QT_TRANSLATE_NOOP("Grammar", "neg."), QT_TRANSLATE_NOOP("Grammar", "passive"), QT_TRANSLATE_NOOP("Grammar", "causative"), QT_TRANSLATE_NOOP("Grammar", "passive causative"), QT_TRANSLATE_NOOP("Grammar", "noun base"),
            QT_TRANSLATE_NOOP("Grammar", "intent"), QT_TRANSLATE_NOOP("Grammar", "3rd intent"), QT_TRANSLATE_NOOP("Grammar", "appearance"), QT_TRANSLATE_NOOP("Grammar", "conjunctive"), QT_TRANSLATE_NOOP("Grammar", "past"),
            QT_TRANSLATE_NOOP("Grammar", "-tara conditional"), QT_TRANSLATE_NOOP("Grammar", "partial conjunctive"), QT_TRANSLATE_NOOP("Grammar", "-ba conditional"), QT_TRANSLATE_NOOP("Grammar", "potential"),
            QT_TRANSLATE_NOOP("Grammar", "command"), QT_TRANSLATE_NOOP("Grammar", "volitional / tentative"), QT_TRANSLATE_NOOP("Grammar", "suru base"), QT_TRANSLATE_NOOP("Grammar", "potential / passive"),
            //: conjugation
            QT_TRANSLATE_NOOP("Grammar", "adverb"), QT_TRANSLATE_NOOP("Grammar", "noun adj."), QT_TRANSLATE_NOOP("Grammar", "-na adj."), QT_TRANSLATE_NOOP("Grammar", "continuous"), QT_TRANSLATE_NOOP("Grammar", "must / neg. -ba cond."),
            QT_TRANSLATE_NOOP("Grammar", "finished / bothersome"), QT_TRANSLATE_NOOP("Grammar", "without / neg. request"), QT_TRANSLATE_NOOP("Grammar", "without / neg. conjunctive"),
            QT_TRANSLATE_NOOP("Grammar", "archaic neg."), QT_TRANSLATE_NOOP("Grammar", "imperative"), QT_TRANSLATE_NOOP("Grammar", "do in advance / keep ~ing")
        };

        return zLang->translate("Grammar", winflectiontext[infl]);
    }

    QString wordTypesText(int type)
    {
        QString result;

        for (int ix = 0; type != 0 && ix != (int)WordTypes::Count; ++ix)
        {
            if ((type & (1 << ix)) != 0)
            {
                if (!result.isEmpty())
                    result += " ";
                result += wordType(ix);
            }
        }

        return result;
    }

    QString wordTypesTextLong(int type, QString separator)
    {
        QString result;

        for (int ix = 0; type != 0 && ix != (int)WordTypes::Count; ++ix)
        {
            if ((type & (1 << ix)) != 0)
            {
                if (!result.isEmpty())
                    result += separator;
                result += wordTypeLong(ix);
            }
        }

        return result;
    }

    QString wordNotesText(int note)
    {
        QString result;

        for (int ix = 0; note != 0 && ix != (int)WordNotes::Count; ++ix)
        {
            if ((note & (1 << ix)) != 0)
            {
                if (!result.isEmpty())
                    result += " ";
                result += wordNote(ix);
            }
        }

        return result;
    }

    QString wordNotesTextLong(int note, QString separator)
    {
        QString result;

        for (int ix = 0; note != 0 && ix != (int)WordNotes::Count; ++ix)
        {
            if ((note & (1 << ix)) != 0)
            {
                if (!result.isEmpty())
                    result += separator;
                result += wordNoteLong(ix);
            }
        }

        return result;
    }

    QString wordFieldsText(int field)
    {
        QString result;

        for (int ix = 0; field != 0 && ix != (int)WordFields::Count; ++ix)
        {
            if ((field & (1 << ix)) != 0)
            {
                if (!result.isEmpty())
                    result += " ";
                result += wordField(ix);
            }
        }

        return result;
    }

    QString wordFieldsTextLong(int field, QString separator)
    {
        QString result;

        for (int ix = 0; field != 0 && ix != (int)WordFields::Count; ++ix)
        {
            if ((field & (1 << ix)) != 0)
            {
                if (!result.isEmpty())
                    result += separator;
                result += wordFieldLong(ix);
            }
        }

        return result;
    }

    QString wordDialectsText(int dia)
    {
        QString result;

        for (int ix = 0; dia != 0 && ix != (int)WordDialects::Count; ++ix)
        {
            if ((dia & (1 << ix)) != 0)
            {
                if (!result.isEmpty())
                    result += " ";
                result += wordDialect(ix);
            }
        }

        return result;
    }

    QString wordDialectsTextLong(int dia, QString separator)
    {
        QString result;

        for (int ix = 0; dia != 0 && ix != (int)WordDialects::Count; ++ix)
        {
            if ((dia & (1 << ix)) != 0)
            {
                if (!result.isEmpty())
                    result += separator;
                result += wordDialectLong(ix);
            }
        }

        return result;
    }

    QString wordInfoText(int inf)
    {
        QString result;

        for (int ix = 0; inf != 0 && ix != (int)WordInfo::Count; ++ix)
        {
            if ((inf & (1 << ix)) != 0)
            {
                if (!result.isEmpty())
                    result += " ";
                result += wordInfo(ix);
            }
        }

        return result;
    }

    QString wordInflectionText(const std::vector<InfTypes> &infl)
    {
        QString result;

        for (int ix = 0, siz = tosigned(infl.size()); ix != siz; ++ix)
        {
            if (ix != 0)
                result += ", ";
            result += wordInflection((uchar)infl[ix]);
        }

        return result;
    }

}


//-------------------------------------------------------------

