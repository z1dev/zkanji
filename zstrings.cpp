/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#include <QStringBuilder>
#include "zstrings.h"
#include "grammar_enums.h"

//-------------------------------------------------------------


//namespace
//{
//    const uchar wordtypecount = 23;
//    const uchar wordnotecount = 21;
//    const uchar wordfieldcount = 8;
//    const uchar worddialectcount = 8;
//    const uchar wordinflcount = 33;
//}

QString Strings::capitalize(const QString &str)
{
    return str[0].toUpper() % str.mid(1);

}

QString Strings::wordType(uchar type)
{
    static const QString texts[(int)WordTypes::Count] = {
        qApp->translate("Grammar", "n"),       qApp->translate("Grammar", "-ru"),      qApp->translate("Grammar", "-u"),           qApp->translate("Grammar", "suru"),
        qApp->translate("Grammar", "-suru"),    qApp->translate("Grammar", "-aru"),     qApp->translate("Grammar", "-kuru"),        qApp->translate("Grammar", "-iku"),
        qApp->translate("Grammar", "-ri"),      qApp->translate("Grammar", "-zuru"),    qApp->translate("Grammar", "tr"),          qApp->translate("Grammar", "intr"),
        qApp->translate("Grammar", "a"),       qApp->translate("Grammar", "na"),       qApp->translate("Grammar", "-to"),          qApp->translate("Grammar", "pn-a"),
        qApp->translate("Grammar", "no"),       qApp->translate("Grammar", "adv"),     qApp->translate("Grammar", "aux"),         qApp->translate("Grammar", "pref"),
        qApp->translate("Grammar", "suf"),     qApp->translate("Grammar", "conj"),    qApp->translate("Grammar", "int"),         qApp->translate("Grammar", "exp"),
        qApp->translate("Grammar", "pn"),    qApp->translate("Grammar", "par"),     qApp->translate("Grammar", "cnt"),         qApp->translate("Grammar", "da/desu"),
        qApp->translate("Grammar", "arch v"),  qApp->translate("Grammar", "arch a"),  qApp->translate("Grammar", "arch -na")
    };

    return texts[type];
}

QString Strings::wordTypeLong(uchar type)
{
    static const QString texts[(int)WordTypes::Count] = {
        qApp->translate("Grammar", "noun"), qApp->translate("Grammar", "ichidan verb"), qApp->translate("Grammar", "godan verb"), qApp->translate("Grammar", "verb with suru"),
        qApp->translate("Grammar", "-suru verb"), qApp->translate("Grammar", "-aru verb"), qApp->translate("Grammar", "-kuru verb"), qApp->translate("Grammar", "-iku verb"),
        qApp->translate("Grammar", "-ri verb"), qApp->translate("Grammar", "-zuru verb"), qApp->translate("Grammar", "transitive verb"), qApp->translate("Grammar", "intransitive verb"),
        qApp->translate("Grammar", "true adjective (-i)"), qApp->translate("Grammar", "adjectival noun (-na)"), qApp->translate("Grammar", "taru adjective (-to)"), qApp->translate("Grammar", "pre-noun adjective"),
        qApp->translate("Grammar", "may take no"), qApp->translate("Grammar", "adverb"), qApp->translate("Grammar", "auxiliary"), qApp->translate("Grammar", "prefix"),
        qApp->translate("Grammar", "suffix"), qApp->translate("Grammar", "conjunction"), qApp->translate("Grammar", "interjection"), qApp->translate("Grammar", "expression"),
        qApp->translate("Grammar", "pronoun"), qApp->translate("Grammar", "particle"), qApp->translate("Grammar", "counter"), qApp->translate("Grammar", "copula da"),
        qApp->translate("Grammar", "archaic verb"), qApp->translate("Grammar", "archaic adjective"), qApp->translate("Grammar", "archaic adjectival noun")
    };

    return texts[type];
}

QString Strings::wordNote(uchar note)
{
    static const QString texts[(int)WordNotes::Count] = {
        qApp->translate("Grammar", "kana"),     qApp->translate("Grammar", "abbr"),    qApp->translate("Grammar", "4k"),   qApp->translate("Grammar", "idiom"),
        qApp->translate("Grammar", "obsc"),    qApp->translate("Grammar", "obso"),    qApp->translate("Grammar", "onom"),    qApp->translate("Grammar", "pv"),
        qApp->translate("Grammar", "rare"),     qApp->translate("Grammar", "sens"),    qApp->translate("Grammar", "col"),     qApp->translate("Grammar", "fam"),
        qApp->translate("Grammar", "hon"),     qApp->translate("Grammar", "hum"),     qApp->translate("Grammar", "pol"),     qApp->translate("Grammar", "arch"),
        qApp->translate("Grammar", "poet"),    qApp->translate("Grammar", "chl"),     qApp->translate("Grammar", "male"),       qApp->translate("Grammar", "fem"),
        qApp->translate("Grammar", "joke"),     qApp->translate("Grammar", "sl"),      qApp->translate("Grammar", "msl"),     qApp->translate("Grammar", "der"),
        qApp->translate("Grammar", "vul"),    
    };

    return texts[note];
}

QString Strings::wordNoteLong(uchar note)
{
    static const QString texts[(int)WordNotes::Count] = {
        qApp->translate("Grammar", "kana usage (usually no kanji)"), qApp->translate("Grammar", "abbreviation"), qApp->translate("Grammar", "four kanji idiom"), qApp->translate("Grammar", "idiomatism"),
        qApp->translate("Grammar", "obscure usage"), qApp->translate("Grammar", "obsolete term"), qApp->translate("Grammar", "onomatopoeia"), qApp->translate("Grammar", "proverb"),
        qApp->translate("Grammar", "rare word or phrase"), qApp->translate("Grammar", "sensitivity"), qApp->translate("Grammar", "colloquialism"), qApp->translate("Grammar", "familiar language"),
        qApp->translate("Grammar", "honorific language"), qApp->translate("Grammar", "humble language"), qApp->translate("Grammar", "polite language"), qApp->translate("Grammar", "archaic expression"),
        qApp->translate("Grammar", "poetic expression"), qApp->translate("Grammar", "children's language"), qApp->translate("Grammar", "male language"), qApp->translate("Grammar", "female language"),
        qApp->translate("Grammar", "joking style"), qApp->translate("Grammar", "slang"), qApp->translate("Grammar", "manga slang"), qApp->translate("Grammar", "derogatory expression"),
        qApp->translate("Grammar", "vulgar expression"),
    };

    return texts[note];
}

QString Strings::wordField(uchar field)
{
    static const QString texts[(int)WordFields::Count] = {
        qApp->translate("Grammar", "archi"), qApp->translate("Grammar", "bus"), qApp->translate("Grammar", "comp"), qApp->translate("Grammar", "econ"),
        qApp->translate("Grammar", "engi"), qApp->translate("Grammar", "fin"), qApp->translate("Grammar", "food"), qApp->translate("Grammar", "law"),
        qApp->translate("Grammar", "mil"), qApp->translate("Grammar", "mus"), 
        
        qApp->translate("Grammar", "anat"), qApp->translate("Grammar", "astr"), qApp->translate("Grammar", "biol"), qApp->translate("Grammar", "bot"),
        qApp->translate("Grammar", "chem"), qApp->translate("Grammar", "geol"), qApp->translate("Grammar", "geom"), qApp->translate("Grammar", "ling"),
        qApp->translate("Grammar", "math"), qApp->translate("Grammar", "med"), qApp->translate("Grammar", "phys"), qApp->translate("Grammar", "zoo"),

        qApp->translate("Grammar", "Buddh"), qApp->translate("Grammar", "Shinto"),

        qApp->translate("Grammar", "baseb"), qApp->translate("Grammar", "mahj"), qApp->translate("Grammar", "m.a."), qApp->translate("Grammar", "shogi"),
        qApp->translate("Grammar", "sports"), qApp->translate("Grammar", "sumo")
    };

    return texts[field];
}

QString Strings::wordFieldLong(uchar field)
{
    static const QString texts[(int)WordFields::Count] = {
        qApp->translate("Grammar", "architecture"), qApp->translate("Grammar", "business"), qApp->translate("Grammar", "computing"), qApp->translate("Grammar", "economy"),
        qApp->translate("Grammar", "engineering"), qApp->translate("Grammar", "finance"), qApp->translate("Grammar", "food"), qApp->translate("Grammar", "law"),
        qApp->translate("Grammar", "military"), qApp->translate("Grammar", "music"),

        qApp->translate("Grammar", "anatomy"), qApp->translate("Grammar", "astronomy"), qApp->translate("Grammar", "biology"), qApp->translate("Grammar", "botany"),
        qApp->translate("Grammar", "chemistry"), qApp->translate("Grammar", "geology"), qApp->translate("Grammar", "geometry"), qApp->translate("Grammar", "linguistics"),
        qApp->translate("Grammar", "mathematics"), qApp->translate("Grammar", "medicine"), qApp->translate("Grammar", "physics"), qApp->translate("Grammar", "zoology"),

        qApp->translate("Grammar", "Buddhism"), qApp->translate("Grammar", "Shinto"),

        qApp->translate("Grammar", "baseball"), qApp->translate("Grammar", "mahjong"), qApp->translate("Grammar", "martial arts"), qApp->translate("Grammar", "shogi"),
        qApp->translate("Grammar", "sports"), qApp->translate("Grammar", "sumo")
    };

    return texts[field];
}

QString Strings::wordDialect(uchar dia)
{
    static const QString texts[(int)WordDialects::Count] = {
        qApp->translate("Grammar", "Ho"), qApp->translate("Grammar", "Kans"), qApp->translate("Grammar", "Kant"), qApp->translate("Grammar", "Kyo"),
        qApp->translate("Grammar", "Kyu"), qApp->translate("Grammar", "Na"), qApp->translate("Grammar", "Os"), qApp->translate("Grammar", "Ry"),
        qApp->translate("Grammar", "Tos"), qApp->translate("Grammar", "Tou"), qApp->translate("Grammar", "Ts"), 
    };

    return texts[dia];
}

QString Strings::wordDialectLong(uchar dia)
{
    static const QString texts[(int)WordDialects::Count] = {
        qApp->translate("Grammar", "Hokkaidou dialect"), qApp->translate("Grammar", "Kansai dialect"), qApp->translate("Grammar", "Kantou dialect"), qApp->translate("Grammar", "Kyoto dialect"),
        qApp->translate("Grammar", "Kyuushuu dialect"), qApp->translate("Grammar", "Nagano dialect"), qApp->translate("Grammar", "Osaka dialect"), qApp->translate("Grammar", "Ryuukyuu dialect"),
        qApp->translate("Grammar", "Tosa dialect"), qApp->translate("Grammar", "Touhoku dialect"), qApp->translate("Grammar", "Tsugaru dialect"),
    };

    return texts[dia];
}

QString Strings::wordInfo(uchar inf)
{
    static const QString texts[(int)WordInfo::Count] = {
        qApp->translate("Grammar", "ate"), qApp->translate("Grammar", "gik"), qApp->translate("Grammar", "i.kanji"), qApp->translate("Grammar", "i.kana"),
        qApp->translate("Grammar", "i.oku"), qApp->translate("Grammar", "o.kanji"), qApp->translate("Grammar", "o.kana")
    };

    return texts[inf];
}

QString Strings::wordInfoLong(uchar inf)
{
    static const QString texts[(int)WordInfo::Count] = {
        qApp->translate("Grammar", "ateji"), qApp->translate("Grammar", "gikun"), qApp->translate("Grammar", "irregular kanji"), qApp->translate("Grammar", "irregular kana"),
        qApp->translate("Grammar", "irregular okurigana"), qApp->translate("Grammar", "outdated kanji"), qApp->translate("Grammar", "outdated kana")
    };

    return texts[inf];
}

// Short tags used for export/import.
static const QString infotexts[(int)WordInfo::Count] = {
    "i_ate", "i_gik", "i_ikanji", "i_ikana", "i_ioku", "i_okanji", "i_okana"
};
QString Strings::wordInfoTags(uchar infs)
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

QString Strings::wordTypeTags(uint types)
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
QString Strings::wordNoteTags(uint notes)
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
QString Strings::wordFieldTags(uint fields)
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
QString Strings::wordDialectTags(ushort dials)
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

uchar Strings::wordTagInfo(const QString &str, bool *ok)
{
    return wordTagT<uchar, (int)WordInfo::Count>(infotexts, str, ok);
}

uint Strings::wordTagTypes(const QString &str, bool *ok)
{
    return wordTagT<uint, (int)WordTypes::Count>(typetexts, str, ok);
}

uint Strings::wordTagNotes(const QString &str, bool *ok)
{
    return wordTagT<uint, (int)WordNotes::Count>(notetexts, str, ok);
}

uint Strings::wordTagFields(const QString &str, bool *ok)
{
    return wordTagT<uint, (int)WordFields::Count>(fieldtexts, str, ok);
}

ushort Strings::wordTagDialects(const QString &str, bool *ok)
{
    return wordTagT<ushort, (int)WordDialects::Count>(dialecttexts, str, ok);
}


QString Strings::wordJLPTLevel(uchar lv)
{
    static const QString texts[5] = { "N5", "N4", "N3", "N2", "N1" };

    return texts[lv];
}

QString Strings::wordAttrib(uchar attr)
{
    static const QString texts[(int)WordAttribs::Count] = {
        qApp->translate("Grammar", "Word types"), qApp->translate("Grammar", "Verb types"), qApp->translate("Grammar", "Adjective types"), qApp->translate("Grammar", "Archaic"),
        qApp->translate("Grammar", "Word usage"), qApp->translate("Grammar", "Relation of speaker and listener"), qApp->translate("Grammar", "Style of speech"),
        qApp->translate("Grammar", "Fields of use"), qApp->translate("Grammar", "Sciences"), qApp->translate("Grammar", "Religion"), qApp->translate("Grammar", "Sports"),
        qApp->translate("Grammar", "Dialects"), qApp->translate("Grammar", "Character usage"), qApp->translate("Grammar", "JLPT level")
    };

    return texts[attr];
}

QString Strings::wordInflection(uchar infl)
{
    static const QString winflectiontext[] = {
        qApp->translate("Grammar", "polite"), qApp->translate("Grammar", "polite past"), qApp->translate("Grammar", "polite neg."), qApp->translate("Grammar", "polite volitional / tentative"),
        qApp->translate("Grammar", "neg."), qApp->translate("Grammar", "passive"), qApp->translate("Grammar", "causative"), qApp->translate("Grammar", "passive causative"), qApp->translate("Grammar", "noun base"),
        qApp->translate("Grammar", "intent"), qApp->translate("Grammar", "3rd intent"), qApp->translate("Grammar", "appearance"), qApp->translate("Grammar", "conjunctive"), qApp->translate("Grammar", "past"),
        qApp->translate("Grammar", "-tara conditional"), qApp->translate("Grammar", "partial conjunctive"), qApp->translate("Grammar", "-ba conditional"), qApp->translate("Grammar", "potential"),
        qApp->translate("Grammar", "command"), qApp->translate("Grammar", "volitional / tentative"), qApp->translate("Grammar", "suru base"), qApp->translate("Grammar", "potential / passive"),
        qApp->translate("Grammar", "adverb", "conjugation"), qApp->translate("Grammar", "noun adj."), qApp->translate("Grammar", "-na adj."), qApp->translate("Grammar", "continuous"), qApp->translate("Grammar", "must / neg. -ba cond."),
        qApp->translate("Grammar", "finished / bothersome"), qApp->translate("Grammar", "without / neg. request"), qApp->translate("Grammar", "without / neg. conjunctive"),
        qApp->translate("Grammar", "archaic neg."), qApp->translate("Grammar", "imperative"), qApp->translate("Grammar", "do in advance / keep ~ing")
    };

    return winflectiontext[infl];
}



QString Strings::wordTypesText(int type)
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

QString Strings::wordTypesTextLong(int type, QString separator)
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

QString Strings::wordNotesText(int note)
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

QString Strings::wordNotesTextLong(int note, QString separator)
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

QString Strings::wordFieldsText(int field)
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

QString Strings::wordFieldsTextLong(int field, QString separator)
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

QString Strings::wordDialectsText(int dia)
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

QString Strings::wordDialectsTextLong(int dia, QString separator)
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

QString Strings::wordInfoText(int inf)
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

QString Strings::wordInflectionText(std::vector<InfTypes> infl)
{
    QString result;

    for (int ix = 0; ix != infl.size(); ++ix)
    {
        if (ix != 0)
            result += ", ";
        result += wordInflection((uchar)infl[ix]);
    }
    
    return result;
}


//-------------------------------------------------------------

