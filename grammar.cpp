/*
** Copyright 2007-2013, 2017-2018 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#include "grammar.h"
#include "grammar_enums.h"
#include "romajizer.h"
#include "zkanjimain.h"

#include "checked_cast.h"


InflectionForm::InflectionForm() : infsize(0), type(WordTypes::Count)
{
    ;
}

InflectionForm::InflectionForm(const QString &form, int infsize, WordTypes type, const std::vector<InfTypes> &inf) : form(form), infsize(infsize), type(type), inf(inf)
{
    ;
}

// Inflections that can alter adjectives.
const char *adjinfromaji[] { "ku", "kunai", "katta", "kattara", "kattari", "kute", "karou", "kereba", "kerya", "sa", "sou" };
QCharStringList adjinf;
const InfTypes adjinft[] { InfTypes::Ku, InfTypes::Nai, InfTypes::Ta, InfTypes::Tara, InfTypes::Tari, InfTypes::Te,
    InfTypes::Ou, InfTypes::Eba, InfTypes::Eba, InfTypes::Sa, InfTypes::Sou };

// When the deinflected adjective ends with the following strings it's invalid, because these forms don't inflect normally.
// Ii is an exception, it's only invalid when the whole result is that word.
const char *invalidadjromaji[] { "koii", "uii", "teii", /*"waii", */"chiii", "doii", "maii", "moii", "noii", "gaii", "niii", /* Must come last */ "ii" };
QCharStringList invalidadj;


const char *adjyoiromaji[] { "koyoi", "uyoi", "teyoi", /*"wayoi", */"chiyoi", "doyoi", "mayoi", "moyoi", "noyoi", "gayoi", "niyoi", /* Must come last */ "yoi" };
QCharStringList adjyoi;

// Forms reached after removing the -sou inflection from nasasou or yosasou, which need special handling
const char *nasaiyosairomaji[] { "nasai", "yosai" };
QCharStringList nasaiyosai;

const char *ikuinfromaji[]
{
    "kimasu", "kimashita", "kimasen'", "kimashou", "kanai", "kaserareru", "kareru", "kaseru",
    "ki", "kitai", "kitagaru", "kisou", "tte", "tta", "ttara", "ttari",
    "keba", "keru", "ke", "kou", "tteru", "tteiru", "kanakuya", "kanakya",
    "cchau", "kanaide", "kazu", "kanu", "kasareru", "cchimau", "ttenasai", "tteoku",
    "ttoku"
};
QCharStringList ikuinf;

const InfTypes ikuinftype[]
{
    InfTypes::Masu, InfTypes::Mashita, InfTypes::Masen, InfTypes::Mashou, InfTypes::Nai, InfTypes::Serareru, InfTypes::Reru, InfTypes::Seru,
    InfTypes::I, InfTypes::Tai, InfTypes::Tagaru, InfTypes::Sou, InfTypes::Te, InfTypes::Ta, InfTypes::Tara, InfTypes::Tari,
    InfTypes::Eba, InfTypes::Eru, InfTypes::E, InfTypes::Ou, InfTypes::Teru, InfTypes::Teru, InfTypes::Kya, InfTypes::Kya,
    InfTypes::Chau, InfTypes::Naide, InfTypes::Zu, InfTypes::Nu, InfTypes::Serareru, InfTypes::Chau, InfTypes::Tenasai, InfTypes::Teoku,
    InfTypes::Teoku
};

const char *suruinfromaji[]
{
    "shimasu", "shimashita", "shimasen'", "shimashou", "shinai", "saserareru", "sareru", "saseru",
    "shi", "shitai", "shitagaru", "shisou", "shite", "shita", "shitara", "shitari",
    "sureba", "shiyo", "seyo", "shiyou", "suru", "shiteru", "shiteiru", "shinakuya",
    "shinakya", "shichau", "shinaide", "sezu", "senu", "sareru", "shichimau", "shitenasai",
    "shiteoku", "shitoku", "surya"
};
QCharStringList suruinf;

const InfTypes suruinftype[]
{
    InfTypes::Masu, InfTypes::Mashita, InfTypes::Masen, InfTypes::Mashou, InfTypes::Nai, InfTypes::Serareru, InfTypes::Reru, InfTypes::Seru,
    InfTypes::I, InfTypes::Tai, InfTypes::Tagaru, InfTypes::Sou, InfTypes::Te, InfTypes::Ta, InfTypes::Tara, InfTypes::Tari,
    InfTypes::Eba, InfTypes::E, InfTypes::E, InfTypes::Ou, InfTypes::Suru, InfTypes::Teru, InfTypes::Teru, InfTypes::Kya,
    InfTypes::Kya, InfTypes::Chau, InfTypes::Naide, InfTypes::Zu, InfTypes::Nu, InfTypes::Serareru, InfTypes::Chau, InfTypes::Tenasai,
    InfTypes::Teoku, InfTypes::Teoku, InfTypes::Eba
};


const char *kuruinfromaji[]
{
    "kimasu", "kimashita", "kimasen'", "kimashou",
    "konai", "kosaserareru", "korareru", "kosaseru",
    "kitai", "kitagaru", "kisou", "kite",
    "kita", "kitara", "kitari", "kureba",
    "koi", "koyou", "kiteru", "kiteiru",
    "konakuya", "konakya", "kichau", "konaide",
    "kozu", "konu", "kichimau", "koreru",
    "kitenasai", "kiteoku", "kitoku", "kurya"
};
QCharStringList kuruinf;

const InfTypes kuruinftype[]
{
    InfTypes::Masu, InfTypes::Mashita, InfTypes::Masen, InfTypes::Mashou,
    InfTypes::Nai, InfTypes::Serareru, InfTypes::Rareru, InfTypes::Seru,
    InfTypes::Tai, InfTypes::Tagaru, InfTypes::Sou, InfTypes::Te,
    InfTypes::Ta, InfTypes::Tara, InfTypes::Tari, InfTypes::Eba,
    InfTypes::E, InfTypes::Ou, InfTypes::Teru, InfTypes::Teru,
    InfTypes::Kya, InfTypes::Kya, InfTypes::Chau, InfTypes::Naide,
    InfTypes::Zu, InfTypes::Nu, InfTypes::Chau, InfTypes::Eru,
    InfTypes::Tenasai, InfTypes::Teoku, InfTypes::Teoku, InfTypes::Eba
};

const char *uinfromaji[]
{
    "imasu", "imashita", "imasen'", "imashou",
    "wanai", "waserareru", "wareru", "waseru",
    "i", "itai", "itagaru", "isou",
    "tte", "tta", "ttara", "ttari",
    "eba", "eru", "e", "ou",
    "tteru", "tteiru", "wanakuya", "wanakya",
    "cchau", "wanaide", "wazu", "wanu",
    "wasareru", "cchimau", "ttenasai", "tteoku",
    "ttoku"
};
QCharStringList uinf;

const InfTypes uinftype[]
{
    InfTypes::Masu, InfTypes::Mashita, InfTypes::Masen, InfTypes::Mashou,
    InfTypes::Nai, InfTypes::Serareru, InfTypes::Reru, InfTypes::Seru,
    InfTypes::I, InfTypes::Tai, InfTypes::Tagaru, InfTypes::Sou,
    InfTypes::Te, InfTypes::Ta, InfTypes::Tara, InfTypes::Tari,
    InfTypes::Eba, InfTypes::Eru, InfTypes::E, InfTypes::Ou,
    InfTypes::Teru, InfTypes::Teru, InfTypes::Kya, InfTypes::Kya,
    InfTypes::Chau, InfTypes::Naide, InfTypes::Zu, InfTypes::Nu,
    InfTypes::Serareru, InfTypes::Chau, InfTypes::Tenasai, InfTypes::Teoku,
    InfTypes::Teoku
};

const char *kuinfromaji[]
{
    "kimasu", "kimashita", "kimasen'", "kimashou",
    "kanai", "kaserareru", "kareru", "kaseru",
    "ki", "kitai", "kitagaru", "kisou",
    "ite", "ita", "itara", "itari",
    "keba", "keru", "ke", "kou",
    "iteru", "iteiru", "kanakuya", "kanakya",
    "ichau", "kanaide", "kazu", "kanu",
    "kasareru", "ichimau", "itenasai", "iteoku",
    "itoku"
};
QCharStringList kuinf;

const InfTypes kuinftype[]
{
    InfTypes::Masu, InfTypes::Mashita, InfTypes::Masen, InfTypes::Mashou,
    InfTypes::Nai, InfTypes::Serareru, InfTypes::Reru, InfTypes::Seru,
    InfTypes::I, InfTypes::Tai, InfTypes::Tagaru, InfTypes::Sou,
    InfTypes::Te, InfTypes::Ta, InfTypes::Tara, InfTypes::Tari,
    InfTypes::Eba, InfTypes::Eru, InfTypes::E, InfTypes::Ou,
    InfTypes::Teru, InfTypes::Teru, InfTypes::Kya, InfTypes::Kya,
    InfTypes::Chau, InfTypes::Naide, InfTypes::Zu, InfTypes::Nu,
    InfTypes::Serareru, InfTypes::Chau, InfTypes::Tenasai, InfTypes::Teoku,
    InfTypes::Teoku
};

const char *guinfromaji[]
{
    "gimasu", "gimashita", "gimasen'", "gimashou",
    "ganai", "gaserareru", "gareru", "gaseru",
    "gi", "gitai", "gitagaru", "gisou",
    "ide", "ida", "idara", "idari",
    "geba", "geru", "ge", "gou",
    "ideru", "ideiru", "ganakuya", "ganakya",
    "ijau", "ganaide", "gazu", "ganu",
    "gasareru", "ijimau", "idenasai", "ideoku",
    "idoku"
};
QCharStringList guinf;

const InfTypes guinftype[]
{
    InfTypes::Masu, InfTypes::Mashita, InfTypes::Masen, InfTypes::Mashou,
    InfTypes::Nai, InfTypes::Serareru, InfTypes::Reru, InfTypes::Seru,
    InfTypes::I, InfTypes::Tai, InfTypes::Tagaru, InfTypes::Sou,
    InfTypes::Te, InfTypes::Ta, InfTypes::Tara, InfTypes::Tari,
    InfTypes::Eba, InfTypes::Eru, InfTypes::E, InfTypes::Ou,
    InfTypes::Teru, InfTypes::Teru, InfTypes::Kya, InfTypes::Kya,
    InfTypes::Chau, InfTypes::Naide, InfTypes::Zu, InfTypes::Nu,
    InfTypes::Serareru, InfTypes::Chau, InfTypes::Tenasai, InfTypes::Teoku,
    InfTypes::Teoku
};

const char *suinfromaji[]
{
    "shimasu", "shimashita", "shimasen'", "shimashou",
    "sanai", "saserareru", "sareru", "saseru",
    "shi", "shitai", "shitagaru", "shisou",
    "shite", "shita", "shitara", "shitari",
    "seba", "seru", "se", "sou",
    "shiteru", "shiteiru", "sanakuya", "sanakya",
    "shichau", "sanaide", "sazu", "sanu",
    "shichimau", "shitenasai", "shiteoku", "shitoku"
};
QCharStringList suinf;

const InfTypes suinftype[]
{
    InfTypes::Masu, InfTypes::Mashita, InfTypes::Masen, InfTypes::Mashou,
    InfTypes::Nai, InfTypes::Serareru, InfTypes::Reru, InfTypes::Seru,
    InfTypes::I, InfTypes::Tai, InfTypes::Tagaru, InfTypes::Sou,
    InfTypes::Te, InfTypes::Ta, InfTypes::Tara, InfTypes::Tari,
    InfTypes::Eba, InfTypes::Eru, InfTypes::E, InfTypes::Ou,
    InfTypes::Teru, InfTypes::Teru, InfTypes::Kya, InfTypes::Kya,
    InfTypes::Chau, InfTypes::Naide, InfTypes::Zu, InfTypes::Nu,
    InfTypes::Chau, InfTypes::Tenasai, InfTypes::Teoku, InfTypes::Teoku
};

const char *tuinfromaji[]
{
    "chimasu", "chimashita", "chimasen'", "chimashou",
    "tanai", "taserareru", "tareru", "taseru",
    "chi", "chitai", "chitagaru", "chisou",
    "tte", "tta", "ttara", "ttari",
    "teba", "teru", "te", "tou",
    "tteru", "tteiru", "tanakuya", "tanakya",
    "cchau", "tanaide", "tazu", "tanu",
    "tasareru", "cchimau", "ttenasai", "tteoku",
    "ttoku"
};
QCharStringList tuinf;

const InfTypes tuinftype[]
{
    InfTypes::Masu, InfTypes::Mashita, InfTypes::Masen, InfTypes::Mashou,
    InfTypes::Nai, InfTypes::Serareru, InfTypes::Reru, InfTypes::Seru,
    InfTypes::I, InfTypes::Tai, InfTypes::Tagaru, InfTypes::Sou,
    InfTypes::Te, InfTypes::Ta, InfTypes::Tara, InfTypes::Tari,
    InfTypes::Eba, InfTypes::Eru, InfTypes::E, InfTypes::Ou,
    InfTypes::Teru, InfTypes::Teru, InfTypes::Kya, InfTypes::Kya,
    InfTypes::Chau, InfTypes::Naide, InfTypes::Zu, InfTypes::Nu,
    InfTypes::Serareru, InfTypes::Chau, InfTypes::Tenasai, InfTypes::Teoku,
    InfTypes::Teoku
};

const char *nuinfromaji[]
{
    "nimasu", "nimashita", "nimasen'", "nimashou",
    "nanai", "naserareru", "nareru", "naseru",
    "ni", "nitai", "nitagaru", "nisou",
    "n'de", "n'da", "n'dara", "n'dari",
    "neba", "neru", "ne", "nou",
    "n'deru", "n'deiru", "nanakuya", "nanakya",
    "n'jau", "nanaide", "nazu", "nanu",
    "nasareru", "n'jimau", "n'denasai", "n'deoku",
    "n'doku"
};
QCharStringList nuinf;

const InfTypes nuinftype[]
{
    InfTypes::Masu, InfTypes::Mashita, InfTypes::Masen, InfTypes::Mashou,
    InfTypes::Nai, InfTypes::Serareru, InfTypes::Reru, InfTypes::Seru,
    InfTypes::I, InfTypes::Tai, InfTypes::Tagaru, InfTypes::Sou,
    InfTypes::Te, InfTypes::Ta, InfTypes::Tara, InfTypes::Tari,
    InfTypes::Eba, InfTypes::Eru, InfTypes::E, InfTypes::Ou,
    InfTypes::Teru, InfTypes::Teru, InfTypes::Kya, InfTypes::Kya,
    InfTypes::Chau, InfTypes::Naide, InfTypes::Zu, InfTypes::Nu,
    InfTypes::Serareru, InfTypes::Chau, InfTypes::Tenasai, InfTypes::Teoku,
    InfTypes::Teoku
};

const char *buinfromaji[]
{
    "bimasu", "bimashita", "bimasen'", "bimashou",
    "banai", "baserareru", "bareru", "baseru",
    "bi", "bitai", "bitagaru", "bisou",
    "n'de", "n'da", "n'dara", "n'dari",
    "beba", "beru", "be", "‚bou",
    "n'deru", "n'deiru", "banakuya", "banakya",
    "n'jau", "banaide", "bazu", "banu",
    "basareru", "n'jimau", "n'denasai", "n'deoku",
    "n'doku"
};
QCharStringList buinf;

const InfTypes buinftype[]
{
    InfTypes::Masu, InfTypes::Mashita, InfTypes::Masen, InfTypes::Mashou,
    InfTypes::Nai, InfTypes::Serareru, InfTypes::Reru, InfTypes::Seru,
    InfTypes::I, InfTypes::Tai, InfTypes::Tagaru, InfTypes::Sou,
    InfTypes::Te, InfTypes::Ta, InfTypes::Tara, InfTypes::Tari,
    InfTypes::Eba, InfTypes::Eru, InfTypes::E, InfTypes::Ou,
    InfTypes::Teru, InfTypes::Teru, InfTypes::Kya, InfTypes::Kya,
    InfTypes::Chau, InfTypes::Naide, InfTypes::Zu, InfTypes::Nu,
    InfTypes::Serareru, InfTypes::Chau, InfTypes::Tenasai, InfTypes::Teoku,
    InfTypes::Teoku
};

const char *muinfromaji[]
{
    "mimasu", "mimashita", "mimasen'", "mimashou",
    "manai", "maserareru", "mareru", "maseru",
    "mi", "mitai", "mitagaru", "misou",
    "n'de", "n'da", "n'dara", "n'dari",
    "meba", "meru", "me", "mou",
    "n'deru", "n'deiru", "manakuya", "manakya",
    "n'jau", "manaide", "mazu", "manu",
    "masareru", "n'jimau", "n'denasai", "n'deoku",
    "n'doku"
};
QCharStringList muinf;

const InfTypes muinftype[]
{
    InfTypes::Masu, InfTypes::Mashita, InfTypes::Masen, InfTypes::Mashou,
    InfTypes::Nai, InfTypes::Serareru, InfTypes::Reru, InfTypes::Seru,
    InfTypes::I, InfTypes::Tai, InfTypes::Tagaru, InfTypes::Sou,
    InfTypes::Te, InfTypes::Ta, InfTypes::Tara, InfTypes::Tari,
    InfTypes::Eba, InfTypes::Eru, InfTypes::E, InfTypes::Ou,
    InfTypes::Teru, InfTypes::Teru, InfTypes::Kya, InfTypes::Kya,
    InfTypes::Chau, InfTypes::Naide, InfTypes::Zu, InfTypes::Nu,
    InfTypes::Serareru, InfTypes::Chau, InfTypes::Tenasai, InfTypes::Teoku,
    InfTypes::Teoku
};

const char *r_uinfromaji[]
{
    "rimasu", "rimashita", "rimasen'", "rimashou",
    "ranai", "raserareru", "rareru", "raseru",
    "ri", "ritai", "ritagaru", "risou",
    "tte", "tta", "ttara", "ttari",
    "reba", "reru", "re", "rou",
    "tteru", "tteiru", "ranakuya", "ranakya",
    "cchau", "ranaide", "razu", "ranu",
    "rasareru", "cchimau", "ttenasai", "tteoku",
    "ttoku"
};
QCharStringList r_uinf;

const InfTypes r_uinftype[]
{
    InfTypes::Masu, InfTypes::Mashita, InfTypes::Masen, InfTypes::Mashou,
    InfTypes::Nai, InfTypes::Serareru, InfTypes::Reru, InfTypes::Seru,
    InfTypes::I, InfTypes::Tai, InfTypes::Tagaru, InfTypes::Sou,
    InfTypes::Te, InfTypes::Ta, InfTypes::Tara, InfTypes::Tari,
    InfTypes::Eba, InfTypes::Eru, InfTypes::E, InfTypes::Ou,
    InfTypes::Teru, InfTypes::Teru, InfTypes::Kya, InfTypes::Kya,
    InfTypes::Chau, InfTypes::Naide, InfTypes::Zu, InfTypes::Nu,
    InfTypes::Serareru, InfTypes::Chau, InfTypes::Tenasai, InfTypes::Teoku,
    InfTypes::Teoku
};

const char *ruinfromaji[]
{
    "masu", "mashita", "masen'", "mashou",
    "nai", "saserareru", "rareru", "saseru",
    "tai", "tagaru", "sou", "te",
    "ta", "tara", "tari", "reba",
    "ro", "yo", "you", "teru",
    "teiru", "nakuya", "nakya", "chau",
    "naide", "zu", "nu", "reru",
    "chimau", "tenasai", "teoku", "toku",
    "rya" };
QCharStringList ruinf;

const InfTypes ruinftype[]
{
    InfTypes::Masu, InfTypes::Mashita, InfTypes::Masen, InfTypes::Mashou,
    InfTypes::Nai, InfTypes::Serareru, InfTypes::Rareru, InfTypes::Seru,
    InfTypes::Tai, InfTypes::Tagaru, InfTypes::Sou, InfTypes::Te,
    InfTypes::Ta, InfTypes::Tara, InfTypes::Tari, InfTypes::Eba,
    InfTypes::E, InfTypes::E, InfTypes::Ou, InfTypes::Teru,
    InfTypes::Teru, InfTypes::Kya, InfTypes::Kya, InfTypes::Chau,
    InfTypes::Naide, InfTypes::Zu, InfTypes::Nu, InfTypes::Eru,
    InfTypes::Chau, InfTypes::Tenasai, InfTypes::Teoku, InfTypes::Teoku,
    InfTypes::Eba
};

//const char *zero0romaji[] = { "zero", "rei", "maru" };
//QCharStringList zero0;
//const char *ichi1romaji[] = { "hito", "ichi", "juu", "sen'", "man'", "oku", "issen'", "ichiman'", "ichioku", "WAN'" };
//QCharStringList ichi1;
//const char *ni2romaji[] = { "ni", "futa", "futsu", "hata", "hatsu", "ji", "TSUU", "nijuu", "nihyaku", "nisen'", "niman'", "nioku"  };
//QCharStringList ni2;
//const char *san3romaji[] = { "san'", "zou", "mi", "mixtsu", "SURI-", "sanjuu", "sanbyaku", "sanzen'", "sanman'", "san'oku" };
//QCharStringList san3;
//const char *yon4romaji[] = { "yo", "yoxtsu", "yon'", "shi", "FO-", "shijuu", "shihyaku", "yon'juu", "yon'hyaku", "yon'sen'", "yon'man'", "yon'oku" };
//QCharStringList yon4;
//const char *go5romaji[] = { "go", "i", "itsu", "FAIBU", "gojuu", "gohyaku", "gosen'", "goman'", "gooku" };
//QCharStringList go5;
//const char *roku6romaji[] = { "mu", "muyu", "muyo", "mui", "muxtsu", "roku", "riku", "SHIKKUSU", "rokujuu", "roppyaku", "rokusen'", "rokuman'", "rokuoku" };
//QCharStringList roku6;
//const char *nana7romaji[] = { "nana", "nano", "nanu", "shichi", "SEBEN'", "SEBUN'", "shichijuu", "shichihyaku", "shichisen'", "shichiman'", "shichioku", "nanajuu", "nanahyaku", "nanasen'", "nanaman'", "nanaoku" };
//QCharStringList nana7;
//const char *hachi8romaji[] = { "ya", "yaxtsu", "you", "hachi", "EITO", "hachijuu", "happyaku", "hassen'", "hachiman'", "hachioku" };
//QCharStringList hachi8;
//const char *kyuu9romaji[] = { "kokono", "kyuu", "ku", "NAIN'", "kyuujuu", "kujuu", "kyuuhyaku", "kuhyaku", "kyuusen'", "kyuuman'", "kyuuoku" };
//QCharStringList kyuu9;
//const char *shimekunromaji[] = { "shime", "jime" };
//QCharStringList shimekun;
//const char *onajikunromaji[] = { "onaji", "onajiku" };
//QCharStringList onajikun;
//const char *marukunromaji[] = { "maru", "kansuujiZERO", "juu" };
//QCharStringList marukun;
//const char *andokunromaji[] = { "ANDO" };
//QCharStringList andokun;
//const char *attokunromaji[] = { "ATTO" };
//QCharStringList attokun;
//const char *purasukunromaji[] = { "PURASU" };
//QCharStringList purasukun;
//
//const char *letterAromaji[] = { "E-" };
//QCharStringList letterA;
//const char *letterBromaji[] = { "BI-" };
//QCharStringList letterB;
//const char *letterCromaji[] = { "SHI-" };
//QCharStringList letterC;
//const char *letterDromaji[] = { "DI-" };
//QCharStringList letterD;
//const char *letterEromaji[] = { "I-" };
//QCharStringList letterE;
//const char *letterFromaji[] = { "EFU" };
//QCharStringList letterF;
//const char *letterGromaji[] = { "JI-" };
//QCharStringList letterG;
//const char *letterHromaji[] = { "EICHI", "ECCHI", "HA-" };
//QCharStringList letterH;
//const char *letterIromaji[] = { "AI", "I" };
//QCharStringList letterI;
//const char *letterJromaji[] = { "JIXE-" };
//QCharStringList letterJ;
//const char *letterKromaji[] = { "KE-", "KI-" };
//QCharStringList letterK;
//const char *letterLromaji[] = { "ERU" };
//QCharStringList letterL;
//const char *letterMromaji[] = { "EMU" };
//QCharStringList letterM;
//const char *letterNromaji[] = { "ENU", "N'" };
//QCharStringList letterN;
//const char *letterOromaji[] = { "O", "U" };
//QCharStringList letterO;
//const char *letterPromaji[] = { "PI-", "PE-" };
//QCharStringList letterP;
//const char *letterQromaji[] = { "KYU-", "KU-" };
//QCharStringList letterQ;
//const char *letterRromaji[] = { "A-RU", "ERU", "RE" };
//QCharStringList letterR;
//const char *letterSromaji[] = { "ESU", "SU" };
//QCharStringList letterS;
//const char *letterTromaji[] = { "TEXI-" };
//QCharStringList letterT;
//const char *letterUromaji[] = { "YU-" };
//QCharStringList letterU;
//const char *letterVromaji[] = { "VUXI-", "BUXI" };
//QCharStringList letterV;
//const char *letterWromaji[] = { "DABURUVUXI-", "DABURUBUXI-" };
//QCharStringList letterW;
//const char *letterXromaji[] = { "EKKUSU" };
//QCharStringList letterX;
//const char *letterYromaji[] = { "WAI" };
//QCharStringList letterY;
//const char *letterZromaji[] = { "ZEXI-", "ZEDDO" };
//QCharStringList letterZ;
//// Intentionally empty list for when the above does not apply.
//QCharStringList noreadings;



void initializeRomajiToKana(const char **romaji, QCharStringList &kana, int size)
{
    kana.reserve(size);
    for (int ix = 0; ix != size; ++ix)
        kana.add(toKana(romaji[ix], -1, true).constData());
}

#define INITRK(x) initializeRomajiToKana(x ## romaji, x , sizeof( x ## romaji ) / sizeof(char *))
void initializeDeinflecter()
{
    INITRK(adjinf);
    INITRK(invalidadj);
    INITRK(adjyoi);
    INITRK(nasaiyosai);
    INITRK(ikuinf);
    INITRK(suruinf);
    INITRK(kuruinf);
    INITRK(uinf);
    INITRK(kuinf);
    INITRK(guinf);
    INITRK(suinf);
    INITRK(tuinf);
    INITRK(nuinf);
    INITRK(buinf);
    INITRK(muinf);
    INITRK(r_uinf);
    INITRK(ruinf);


    //INITRK(zero0);
    //INITRK(ichi1);
    //INITRK(ni2);
    //INITRK(san3);
    //INITRK(yon4);
    //INITRK(go5);
    //INITRK(roku6);
    //INITRK(nana7);
    //INITRK(hachi8);
    //INITRK(kyuu9);
    //INITRK(shimekun);
    //INITRK(onajikun);
    //INITRK(marukun);
    //INITRK(andokun);
    //INITRK(attokun);
    //INITRK(purasukun);

    //INITRK(letterA);
    //INITRK(letterB);
    //INITRK(letterC);
    //INITRK(letterD);
    //INITRK(letterE);
    //INITRK(letterF);
    //INITRK(letterG);
    //INITRK(letterH);
    //INITRK(letterI);
    //INITRK(letterJ);
    //INITRK(letterK);
    //INITRK(letterL);
    //INITRK(letterM);
    //INITRK(letterN);
    //INITRK(letterO);
    //INITRK(letterP);
    //INITRK(letterQ);
    //INITRK(letterR);
    //INITRK(letterS);
    //INITRK(letterT);
    //INITRK(letterU);
    //INITRK(letterV);
    //INITRK(letterW);
    //INITRK(letterX);
    //INITRK(letterY);
    //INITRK(letterZ);

}

#undef INITRK

// Conversion from old WordTypes values to WordTypes
static const int OldWordTypes[] =
{
    (int)WordTypes::Noun, (int)WordTypes::TakesSuru, (int)WordTypes::GodanVerb, (int)WordTypes::Expression,
    (int)WordTypes::NaAdj, (int)WordTypes::Transitive, (int)WordTypes::IchidanVerb, (int)WordTypes::Intransitive,
    (int)WordTypes::Adverb, (int)WordTypes::TrueAdj, (int)WordTypes::MayTakeNo, (int)WordTypes::Suffix,
    (int)WordTypes::Taru, (int)WordTypes::Prefix, (int)WordTypes::SuruVerb, (int)WordTypes::PrenounAdj,
    (int)WordTypes::Conjunction, (int)WordTypes::Interjection, (int)WordTypes::IkuVerb, (int)WordTypes::Aux,
    (int)WordTypes::KuruVerb, (int)WordTypes::Aux, (int)WordTypes::AruVerb,
};

uint convertOldTypes(uint old)
{
    int result = 0;
    for (int ix = 0; ix != 23; ++ix)
    {
        if ((old & (1 << ix)) == 0)
            continue;

        result |= 1 << OldWordTypes[ix];

    }

    return result;
}


static const int OldWordNotes[] =
{
    (int)WordNotes::Sensitive, (int)WordNotes::Abbrev, (int)WordNotes::Honorific, (int)WordNotes::Humble,
    (int)WordNotes::Colloquial, (int)WordNotes::Polite, (int)WordNotes::Slang, (int)WordNotes::Familiar,
    (int)WordNotes::Male, (int)WordNotes::Female, (int)WordNotes::KanaOnly, 0 /*(int)WordNotes::KanjiOnly*/,
    (int)WordNotes::Archaic, (int)WordNotes::Rare, (int)WordNotes::Obsolete, (int)WordNotes::Vulgar,
    (int)WordNotes::Vulgar, (int)WordNotes::Derogatory, (int)WordNotes::Obscure, (int)WordNotes::Idiomatic,
    (int)WordNotes::MangaSlang,
};

uint convertOldNotes(uint old)
{
    int result = 0;
    for (int ix = 0; ix != 21; ++ix)
    {
        if ((old & (1 << ix)) == 0)
            continue;

        result |= 1 << OldWordNotes[ix];
    }

    return result;
}

static const int OldWordFields[] =
{
    (int)WordFields::Math,
    (int)WordFields::Linguistics,
    (int)WordFields::Computing,
    (int)WordFields::Food,
    (int)WordFields::MartialArts,
    (int)WordFields::Geometry,
    (int)WordFields::Physics,
    (int)WordFields::Buddhism,
};

uint convertOldFields(uint old)
{
    int result = 0;
    for (int ix = 0; ix != 8; ++ix)
    {
        if ((old & (1 << ix)) == 0)
            continue;

        result |= 1 << OldWordFields[ix];
    }

    return result;
}

void deinflectedForms(QString str, QString hstr, int infsize, std::vector<InfTypes> inf, WordTypes oldtype, smartvector<InflectionForm> &results);

// Both str and hstr contain the same kana word, but hstr is hiraganized for checks. They are still
// in their original inflected forms. Str is kept in case the result should keep the original katakana.
// Newlen is the new length of the deinflected word without the suffix, which is added in this step
// of deinflection. Infsize is the number of characters before deinflection, that were changed
// in previous steps.
void addInflectionVariant(QString str, QString hstr, int newlen, QString suffix, int infsize, WordTypes type, WordTypes oldtype, std::vector<InfTypes> inf, InfTypes inftype, smartvector<InflectionForm> &results)
{
    static const QString dekirukana = toKana("dekiru", 6);
    static const QString irukana = toKana("iru", 3);
    static const QString arukana = toKana("aru", 3);
    static const ushort dekirukanji1arr[] { 0x51FA, 0x6765, 0x308B, 0 };
    static const ushort dekirukanji2arr[] { 0x51FA, 0x4F86, 0x308B, 0 };
    static const QString dekirukanji1 = QString::fromUtf16(dekirukanji1arr);
    static const QString dekirukanji2 = QString::fromUtf16(dekirukanji2arr);

    infsize = std::max(0, infsize - std::max(0, str.size() - newlen)) + suffix.size();

    str = str.left(newlen) + suffix;
    hstr = hstr.left(newlen) + suffix;

    if (str.isEmpty() || (str.size() == 1 && hstr == QChar(MINITSU)) || (infsize == str.size() && type != WordTypes::TrueAdj && type != WordTypes::SuruVerb && type != WordTypes::KuruVerb))
        return;

    if (inf.empty() && inftype == InfTypes::Rareru && (str == dekirukana || str == dekirukanji1 || str == dekirukanji2))
        inftype = InfTypes::Reru;

    // Return for double passive or potential after passive.
    if (!inf.empty() && (inftype == InfTypes::Reru || inftype == InfTypes::Serareru) && (inf.back() == InfTypes::Reru || inf.back() == InfTypes::Rareru))
        return;

    // Return for double potential.
    if (inftype == InfTypes::Eru && ((!inf.empty() && inf.back() == InfTypes::Eru) || str == dekirukana || str == dekirukanji1 || str == dekirukanji2))
        return;

    // Return when we have too many inflections or there is no string left
    if (inf.size() > 50 || (inftype == InfTypes::Suru && type == WordTypes::SuruVerb))
        return;

    //return for bad iru/aru inflections
    if ((inftype == InfTypes::Nu && (str == irukana || str == arukana)) || (inftype == InfTypes::Nai && str == arukana))
        return;

    // Return for bad nu inflections
    if (!inf.empty() && inftype == InfTypes::Nu && inf.back() != InfTypes::Eba)
        return;

    //int lastvar = -1;
    //for (int i = l->count - 1; i >= 0 && lastvar < 0; i--)
    //    if (l->items[i]->inf == inf)
    //        lastvar = (1 << l->items[i]->type);

    // Do nothing for inflection types that must be the first inflection.
    if (!inf.empty() && (inftype == InfTypes::Masu || inftype == InfTypes::Masen || inftype == InfTypes::Mashita ||
        inftype == InfTypes::Mashou || inftype == InfTypes::I || inftype == InfTypes::Tari ||
        inftype == InfTypes::Eba || inftype == InfTypes::E || inftype == InfTypes::Ku ||
        inftype == InfTypes::Sa || inftype == InfTypes::Na || inftype == InfTypes::Kya ||
        (inftype == InfTypes::Suru && inf.back() == InfTypes::Suru) || inftype == InfTypes::Zu ||
        inftype == InfTypes::Ou || inftype == InfTypes::Sou || inftype == InfTypes::Tenasai ||
        (inftype == InfTypes::Suru && oldtype == WordTypes::GodanVerb)))
        return;

    // Return if the tagaru ending is not a result of a previous deinflection that resulted in a godan verb.
    if (!inf.empty() && inftype == InfTypes::Tagaru && oldtype != WordTypes::GodanVerb)
        return;

    // Return if -u inflection found before these inflections
    if ((inftype == InfTypes::Reru || inftype == InfTypes::Seru || inftype == InfTypes::Serareru || inftype == InfTypes::Eru ||
        inftype == InfTypes::Rareru || inftype == InfTypes::Teru) && oldtype == WordTypes::GodanVerb)
        return;

    //types |= 1 << type;

    // It's possible that we reach the same word form and word type with the same inflections,
    // so to avoid infinite loops we do nothing when this happens.
    for (const InflectionForm *form : results)
    {
        if (form->form == str && form->inf == inf && form->type == type)
            return;
    }

    inf.push_back(inftype);

    //results.push_back(new InflectionForm(str, infsize, type, inf));
    results.emplace_back(str, infsize, type, inf);

    if ((type == WordTypes::TakesSuru || type == WordTypes::IchidanVerb || type == WordTypes::GodanVerb ||
        type == WordTypes::TrueAdj /*|| type == WordTypes::AuxAdj*/) && (inftype != InfTypes::I || type != WordTypes::IchidanVerb))
        deinflectedForms(str, hstr, infsize, inf, type, results);
}

void deinflectAdjective(QString str, QString hstr, smartvector<InflectionForm> &results);

// Called recursively to deinflect a word. Str is the current word form that might be deinflected further.
// infl is the list of previous results which gets expanded in every iteration. Type contains the required
// grammatical type of the form passed in str. If type is -1 no type is set.
void deinflectedForms(QString str, QString hstr, int infsize, std::vector<InfTypes> inf, WordTypes oldtype, smartvector<InflectionForm> &results)
{
    static const QString surukana = toKana("suru", 4);
    static const QString kurukana = toKana("kuru", 4);

    if (hstr.isEmpty())
        return;

    if (inf.empty())
        deinflectAdjective(str, hstr, results);

    // If the "inflection" is the -na ending of a na adjective, it is added as the sole possible inflection.
    if (inf.empty() && (hstr.at(hstr.size() - 1).unicode() == 0x306A /* na */ || hstr.at(hstr.size() - 1).unicode() == 0x306B /* ni */ || hstr.at(hstr.size() - 1).unicode() == 0x3067 /* de */))
        addInflectionVariant(str, hstr, hstr.size() - 1, QString(), infsize, WordTypes::NaAdj, oldtype, std::vector<InfTypes>(), hstr.at(hstr.size() - 1).unicode() == 0x306A ? InfTypes::Na : hstr.at(hstr.size() - 1).unicode() == 0x306B ? InfTypes::Ku : InfTypes::Te, results);

    for (int ix = 0, siz = tosigned(ikuinf.size()); ix != siz; ++ix)
        if (hstr.rightRef(ikuinf[ix].size()) == ikuinf[ix])
            addInflectionVariant(str, hstr, hstr.size() - ikuinf[ix].size(), QChar(0x304f) /* ku */, infsize, WordTypes::IkuVerb, oldtype, inf, ikuinftype[ix], results);

    for (int ix = 0, siz = tosigned(suruinf.size()); ix != siz; ++ix)
        if (hstr.rightRef(suruinf[ix].size()) == suruinf[ix])
        {
            addInflectionVariant(str, hstr, hstr.size() - suruinf[ix].size(), surukana, infsize, WordTypes::SuruVerb, oldtype, inf, suruinftype[ix], results);
            addInflectionVariant(str, hstr, hstr.size() - suruinf[ix].size(), QString(), infsize, WordTypes::TakesSuru, oldtype, inf, suruinftype[ix], results);
        }

    if (hstr.size() != 1)
    {
        for (int ix = 0, siz = tosigned(kuruinf.size()); ix != siz; ++ix)
        {
            int len = kuruinf[ix].size();
            if (hstr.size() >= len && qcharncmp(hstr.rightRef(len - 1).constData(), kuruinf[ix].rightData(len - 1), len - 1) == 0)
            {
                if (hstr.at(hstr.size() - len) == kuruinf[ix][0])
                    addInflectionVariant(str, hstr, hstr.size() - len, kurukana, infsize, WordTypes::KuruVerb, oldtype, inf, kuruinftype[ix], results);
                else if (hstr.at(hstr.size() - len) == QChar(0x6765) /* kuru kanji */ || hstr.at(hstr.size() - len) == QChar(0x4F86) /* kuru kanji variant */ )
                    addInflectionVariant(str, hstr, hstr.size() - len + 1, QChar(0x308B) /* ru */, infsize, WordTypes::KuruVerb, oldtype, inf, kuruinftype[ix], results);
            }
        }
    }

    for (int ix = 0, siz = tosigned(uinf.size()); ix != siz; ++ix)
        if (hstr.rightRef(uinf[ix].size()) == uinf[ix])
            addInflectionVariant(str, hstr, hstr.size() - uinf[ix].size(), QChar(0x3046) /* u */, infsize, WordTypes::GodanVerb, oldtype, inf, uinftype[ix], results);

    for (int ix = 0, siz = tosigned(kuinf.size()); ix != siz; ++ix)
        if (hstr.rightRef(kuinf[ix].size()) == kuinf[ix])
            addInflectionVariant(str, hstr, hstr.size() - kuinf[ix].size(), QChar(0x304f) /* ku */, infsize, WordTypes::GodanVerb, oldtype, inf, kuinftype[ix], results);

    for (int ix = 0, siz = tosigned(guinf.size()); ix != siz; ++ix)
        if (hstr.rightRef(guinf[ix].size()) == guinf[ix])
            addInflectionVariant(str, hstr, hstr.size() - guinf[ix].size(), QChar(0x3050) /* gu */, infsize, WordTypes::GodanVerb, oldtype, inf, guinftype[ix], results);

    for (int ix = 0, siz = tosigned(suinf.size()); ix != siz; ++ix)
        if (hstr.rightRef(suinf[ix].size()) == suinf[ix])
            addInflectionVariant(str, hstr, hstr.size() - suinf[ix].size(), QChar(0x3059) /* su */, infsize, WordTypes::GodanVerb, oldtype, inf, suinftype[ix], results);

    for (int ix = 0, siz = tosigned(tuinf.size()); ix != siz; ++ix)
        if (hstr.rightRef(tuinf[ix].size()) == tuinf[ix])
            addInflectionVariant(str, hstr, hstr.size() - tuinf[ix].size(), QChar(0x3064) /* tu */, infsize, WordTypes::GodanVerb, oldtype, inf, tuinftype[ix], results);

    for (int ix = 0, siz = tosigned(nuinf.size()); ix != siz; ++ix)
        if (hstr.rightRef(nuinf[ix].size()) == nuinf[ix])
            addInflectionVariant(str, hstr, hstr.size() - nuinf[ix].size(), QChar(0x306C) /* nu */, infsize, WordTypes::GodanVerb, oldtype, inf, nuinftype[ix], results);

    for (int ix = 0, siz = tosigned(buinf.size()); ix != siz; ++ix)
        if (hstr.rightRef(buinf[ix].size()) == buinf[ix])
            addInflectionVariant(str, hstr, hstr.size() - buinf[ix].size(), QChar(0x3076) /* bu */, infsize, WordTypes::GodanVerb, oldtype, inf, buinftype[ix], results);

    for (int ix = 0, siz = tosigned(muinf.size()); ix != siz; ++ix)
        if (hstr.rightRef(muinf[ix].size()) == muinf[ix])
            addInflectionVariant(str, hstr, hstr.size() - muinf[ix].size(), QChar(0x3080) /* mu */, infsize, WordTypes::GodanVerb, oldtype, inf, muinftype[ix], results);

    for (int ix = 0, siz = tosigned(r_uinf.size()); ix != siz; ++ix)
        if (hstr.rightRef(r_uinf[ix].size()) == r_uinf[ix])
            addInflectionVariant(str, hstr, hstr.size() - r_uinf[ix].size(), QChar(0x308B) /* ru */, infsize, WordTypes::GodanVerb, oldtype, inf, r_uinftype[ix], results);

    if (inf.empty())
        addInflectionVariant(str, hstr, hstr.size(), QChar(0x308B) /* ru */, infsize, WordTypes::IchidanVerb, oldtype, inf, InfTypes::I, results);

    for (int ix = 0, siz = tosigned(ruinf.size()); ix != siz; ++ix)
        if (hstr.rightRef(ruinf[ix].size()) == ruinf[ix])
            addInflectionVariant(str, hstr, hstr.size() - ruinf[ix].size(), QChar(0x308B) /* ru */, infsize, WordTypes::IchidanVerb, oldtype, inf, ruinftype[ix], results);
}

void deinflectAdjective(QString str, QString hstr, smartvector<InflectionForm> &results)
{
    static const ushort iikana[] { 0x3044, 0x3044, 0 };
    static const ushort waiikana[] { 0x308f, 0x3044, 0x3044, 0 };

    std::vector<InfTypes> inflist;

    int infcnt = -1;

    int infsize = 0;

    WordTypes oldtype = WordTypes::Count;

    while (infcnt != tosigned(inflist.size()))
    {
        infcnt = tosigned(inflist.size());

        for (int ix = 0, siz = tosigned(adjinf.size()); ix != siz; ++ix)
        {
            int adjlen = adjinf[ix].size();
            if (hstr.size() <= adjlen || hstr.right(adjlen) != adjinf[ix] || hstr.at(hstr.size() - adjlen).unicode() == MINITSU)
                continue;

            str.replace(str.size() - adjlen, adjlen, QChar(0x3044) /* i */ );
            hstr.replace(hstr.size() - adjlen, adjlen, QChar(0x3044) /* i */);
            infsize = std::max(0, infsize - adjlen) + 1;

            // Special handling for i-adjectives ending with a form of ii. Only yoi can inflect.
            // In case it's *waii (like kawaii) it's another exception as it can't be inflected with sou.

            bool isvalid = true;
            for (int iy = 0, sizy = tosigned(invalidadj.size()); iy != sizy; ++iy)
            {
                if (hstr.rightRef(invalidadj[iy].size()) == invalidadj[iy] && (iy != sizy - 1 || hstr.size() == 2))
                {
                    isvalid = false;
                    break;
                }
            }

            if (isvalid && hstr.rightRef(3) == QString::fromUtf16(waiikana) && adjinft[ix] == InfTypes::Sou)
                isvalid = false;

            if (!isvalid)
                break;

            // Forms reached after removing the -sou inflection from nasasou or yosasou, which need special handling
            if (adjinft[ix] == InfTypes::Sou && hstr.size() > 3 && (hstr.rightRef(3) == nasaiyosai[0] || hstr.rightRef(3) == nasaiyosai[1]))
            {

                // The -nasasou ending becomes -nasai which is the -sou form of -nai. This would result
                // in the word "abunai" listed as being inflected with -sou when "abunasasou" is incorrect.
                // Only allow this ending if there are other inflections affecting the word too.
                bool na = hstr.at(hstr.size() - 3) == 0x306A /* na */;
                addInflectionVariant(str, hstr, hstr.size() - 2, QChar(0x3044), infsize, na ? WordTypes::Count : WordTypes::TrueAdj, oldtype, inflist, adjinft[ix], results);
                if (!na)
                    addInflectionVariant(str, hstr, hstr.size() - 3, QString::fromUtf16(iikana), infsize, WordTypes::TrueAdj, oldtype, inflist, adjinft[ix], results);
                infsize = std::max(0, infsize - adjlen) + 1;

                // Replace -sai with -i to get nai or yoi.
                str.replace(str.size() - 2, 2, QChar(0x3044) /* i */);
                hstr.replace(hstr.size() - 2, 2, QChar(0x3044) /* i */);
                infsize = std::max(0, infsize - 2) + 1;

                oldtype = WordTypes::TrueAdj;

                inflist.push_back(adjinft[ix]);
                continue;
            }

            // Special handling for i-adjectives ending in yoi. Add ii ending words too.
            for (int iy = 0, sizy = tosigned(adjyoi.size()); iy != sizy; ++iy)
                if (hstr.rightRef(adjyoi[iy].size()) == adjyoi[iy] && (iy != sizy - 1 || hstr.size() == 2))
                    addInflectionVariant(str, hstr, hstr.size() - 2, QString::fromUtf16(iikana), infsize, WordTypes::TrueAdj, oldtype, inflist, adjinft[ix], results);

            addInflectionVariant(str, hstr, hstr.size(), QString(), infsize, WordTypes::TrueAdj, oldtype, inflist, adjinft[ix], results);

            oldtype = WordTypes::TrueAdj;
            inflist.push_back(adjinft[ix]);
        }
    }
}

void deinflect(QString str, smartvector<InflectionForm> &result)
{
    //smartvector<InflectionForm> inflections;
    deinflectedForms(str, hiraganize(str), 0, std::vector<InfTypes>(), WordTypes::Count, result);
    //return inflections;
}

