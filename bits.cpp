/*
** Copyright 2007-2013, 2017 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#include <QFile>
#include <QDataStream>
#include "bits.h"

BitArray::BitArray() : base(), bitsize(0)
{

}

BitArray::BitArray(const BitArray &src) : base(src), bitsize(src.bitsize)
{

}

BitArray::BitArray(BitArray &&src) : base(std::move(src)), bitsize(0)
{
    std::swap(bitsize, src.bitsize);
}

BitArray& BitArray::operator=(const BitArray &src)
{
    base::operator=(src);
    bitsize = src.bitsize;
    return *this;
}

BitArray& BitArray::operator=(BitArray &&src)
{
    base::operator=(std::move(src));

    std::swap(bitsize, src.bitsize);
    return *this;
}

void BitArray::load(QDataStream &stream)
{
    quint8 cnt;
    stream >> cnt;
    setSize(cnt);

    if (cnt == 0)
        return;

    stream.readRawData(base::data(), (bitsize + 7) / 8);
}

void BitArray::resize(int size)
{
    if (size == bitsize)
        return;

    base::resize((size + 7) / 8);
    bitsize = size;
}

void BitArray::setSize(int size)
{
    if (size == bitsize)
        return;

    base::setSize((size + 7) / 8);
    bitsize = size;
}

void BitArray::setSize(int size, bool toggle)
{
    if (size == bitsize)
        return;

    base::setSize((size + 7) / 8);
    memset(data(), toggle ? 0xff : 0, (size + 7) / 8);
    bitsize = size;
}

int BitArray::size() const
{
    return bitsize;
}

bool BitArray::toggled(int index) const
{
#ifdef _DEBUG
    if (index < 0)
        throw "index out of bounds.";
#endif
    if (bitsize <= index)
        return false;
    int cpos = index / 8;
    index -= cpos * 8;

    return ((data()[cpos] >> index) & 1) == 1;
}

void BitArray::set(int index, bool toggle)
{
#ifdef _DEBUG
    if (index < 0 || index >= bitsize)
        throw "index out of bounds.";
#endif
    int cpos = index / 8;
    index -= cpos * 8;

    char &c = data()[cpos];
    if (toggle)
        c |= (1 << index);
    else
        c &= ~(1 << index);
}


