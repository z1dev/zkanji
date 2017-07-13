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
        tr("n"),       tr("-ru"),      tr("-u"),           tr("suru"),
        tr("-suru"),    tr("-aru"),     tr("-kuru"),        tr("-iku"),
        tr("-ri"),      tr("-zuru"),    tr("tr"),          tr("intr"),
        tr("a"),       tr("na"),       tr("-to"),          tr("pn-a"),
        tr("no"),       tr("adv"),     tr("aux"),         tr("pref"),
        tr("suf"),     tr("conj"),    tr("int"),         tr("exp"),
        tr("pn"),    tr("par"),     tr("cnt"),         tr("da/desu"),
        tr("arch v"),  tr("arch a"),  tr("arch -na")
    };

    return texts[type];
}

QString Strings::wordTypeLong(uchar type)
{
    static const QString texts[(int)WordTypes::Count] = {
        tr("noun"), tr("ichidan verb"), tr("godan verb"), tr("verb with suru"),
        tr("-suru verb"), tr("-aru verb"), tr("-kuru verb"), tr("-iku verb"),
        tr("-ri verb"), tr("-zuru verb"), tr("transitive verb"), tr("intransitive verb"),
        tr("true adjective (-i)"), tr("adjectival noun (-na)"), tr("taru adjective (-to)"), tr("pre-noun adjective"),
        tr("may take no"), tr("adverb"), tr("auxiliary"), tr("prefix"),
        tr("suffix"), tr("conjunction"), tr("interjection"), tr("expression"),
        tr("pronoun"), tr("particle"), tr("counter"), tr("copula da"),
        tr("archaic verb"), tr("archaic adjective"), tr("archaic adjectival noun")
    };

    return texts[type];
}

QString Strings::wordNote(uchar note)
{
    static const QString texts[(int)WordNotes::Count] = {
        tr("kana"),     tr("abbr"),    tr("4k"),   tr("idiom."),
        tr("obsc"),    tr("obso"),    tr("onom"),    tr("pv."),
        tr("rare"),     tr("sens"),    tr("col"),     tr("fam."),
        tr("hon"),     tr("hum"),     tr("pol"),     tr("arch"),
        tr("poet"),    tr("chl"),     tr("male"),       tr("fem"),
        tr("joke"),     tr("sl"),      tr("msl"),     tr("der"),
        tr("vul"),    
    };

    return texts[note];
}

QString Strings::wordNoteLong(uchar note)
{
    static const QString texts[(int)WordNotes::Count] = {
        tr("kana usage (usually no kanji)"), tr("abbreviation"), tr("four kanji idiom"), tr("idiomatism"),
        tr("obscure usage"), tr("obsolete term"), tr("onomatopoeia"), tr("proverb"),
        tr("rare word or phrase"), tr("sensitivity"), tr("colloquialism"), tr("familiar language"),
        tr("honorific language"), tr("humble language"), tr("polite language"), tr("archaic expression"),
        tr("poetic expression"), tr("children's language"), tr("male language"), tr("female language"),
        tr("joking style"), tr("slang"), tr("manga slang"), tr("derogatory expression"),
        tr("vulgar expression"),
    };

    return texts[note];
}

QString Strings::wordField(uchar field)
{
    static const QString texts[(int)WordFields::Count] = {
        tr("archi"), tr("bus"), tr("comp"), tr("econ"),
        tr("engi"), tr("fin"), tr("food"), tr("law"),
        tr("mil"), tr("mus"), 
        
        tr("anat"), tr("astr"), tr("biol"), tr("bot"),
        tr("chem"), tr("geol"), tr("geom"), tr("ling"),
        tr("math"), tr("med"), tr("phys"), tr("zoo"),

        tr("Buddh"), tr("Shinto"),

        tr("baseb"), tr("mahj"), tr("m.a."), tr("shogi"),
        tr("sports"), tr("sumo")
    };

    return texts[field];
}

QString Strings::wordFieldLong(uchar field)
{
    static const QString texts[(int)WordFields::Count] = {
        tr("architecture"), tr("business"), tr("computing"), tr("economy"),
        tr("engineering"), tr("finance"), tr("food"), tr("law"),
        tr("military"), tr("music"),

        tr("anatomy"), tr("astronomy"), tr("biology"), tr("botany"),
        tr("chemistry"), tr("geology"), tr("geometry"), tr("linguistics"),
        tr("mathematics"), tr("medicine"), tr("physics"), tr("zoology"),

        tr("Buddhism"), tr("Shinto"),

        tr("baseball"), tr("mahjong"), tr("martial arts"), tr("shogi"),
        tr("sports"), tr("sumo")
    };

    return texts[field];
}

QString Strings::wordDialect(uchar dia)
{
    static const QString texts[(int)WordDialects::Count] = {
        tr("Ho"), tr("Kans"), tr("Kant"), tr("Kyo"),
        tr("Kyu"), tr("Na"), tr("Os"), tr("Ry"),
        tr("Tos"), tr("Tou"), tr("Ts"), 
    };

    return texts[dia];
}

QString Strings::wordDialectLong(uchar dia)
{
    static const QString texts[(int)WordDialects::Count] = {
        tr("Hokkaidou dialect"), tr("Kansai dialect"), tr("Kantou dialect"), tr("Kyoto dialect"),
        tr("Kyuushuu dialect"), tr("Nagano dialect"), tr("Osaka dialect"), tr("Ryuukyuu dialect"),
        tr("Tosa dialect"), tr("Touhoku dialect"), tr("Tsugaru dialect"),
    };

    return texts[dia];
}

QString Strings::wordInfo(uchar inf)
{
    static const QString texts[(int)WordInfo::Count] = {
        tr("ate"), tr("gik"), tr("i.kanji"), tr("i.kana"),
        tr("i.oku"), tr("o.kanji"), tr("o.kana")
    };

    return texts[inf];
}

QString Strings::wordInfoLong(uchar inf)
{
    static const QString texts[(int)WordInfo::Count] = {
        tr("ateji"), tr("gikun"), tr("irregular kanji"), tr("irregular kana"),
        tr("irregular okurigana"), tr("outdated kanji"), tr("outdated kana")
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
        tr("Word types"), tr("Verb types"), tr("Adjective types"), tr("Archaic"),
        tr("Word usage"), tr("Relation of speaker and listener"), tr("Style of speech"),
        tr("Fields of use"), tr("Sciences"), tr("Religion"), tr("Sports"),
        tr("Dialects"), tr("Character usage"), tr("JLPT level")
    };

    return texts[attr];
}

QString Strings::wordInflection(uchar infl)
{
    static const QString winflectiontext[] = {
        tr("polite"), tr("polite past"), tr("polite neg."), tr("polite volitional / tentative"),
        tr("neg."), tr("passive"), tr("causative"), tr("passive causative"), tr("noun base"),
        tr("intent"), tr("3rd intent"), tr("appearance"), tr("conjunctive"), tr("past"),
        tr("-tara conditional"), tr("partial conjunctive"), tr("-ba conditional"), tr("potential"),
        tr("command"), tr("volitional / tentative"), tr("suru base"), tr("potential / passive"),
        tr("adverb"), tr("noun adj."), tr("-na adj."), tr("continuous"), tr("must / neg. -ba cond."),
        tr("finished / bothersome"), tr("without / neg. request"), tr("without / neg. conjunctive"),
        tr("archaic neg."), tr("imperative"), tr("do in advance / keep ~ing")
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

QString Strings::wordTypesTextLong(int type)
{
    QString result;

    for (int ix = 0; type != 0 && ix != (int)WordTypes::Count; ++ix)
    {
        if ((type & (1 << ix)) != 0)
        {
            if (!result.isEmpty())
                result += " ";
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

