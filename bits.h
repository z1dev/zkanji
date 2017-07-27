/*
** Copyright 2007-2013, 2017 Sólyom Zoltán
** This file is part of zkanji, a free software released under the terms of the
** GNU General Public License version 3. See the file LICENSE for details.
**/

#ifndef BITS_H
#define BITS_H

#include "fastarray.h"

class QDataStream;
class BitArray : public fastarray<char>
{
public:
    BitArray();
    BitArray(const BitArray &src);
    BitArray(BitArray &&src);
    BitArray& operator=(const BitArray &src);
    BitArray& operator=(BitArray &&src);

    void load(QDataStream &stream);

    // Resizes the bits array to fit at least size bits. Values with indexes
    // above the old size will be uninitialized.
    void resize(int size);
    // Changes the size of the bits array to fit at least size bits. All
    // values in the array can be considered uninitialized after the call.
    void setSize(int size);
    // Changes the size of the bits array, setting the bits to the value of
    // toggle.
    void setSize(int size, bool toggle);
    // Returns the number of bits that the bits array can hold.
    int size() const;
    // Returns whether the bit at index is toggled (has a value of 1).
    bool toggled(int index) const;
    // Changes the bit at index to be toggled or not.
    void set(int index, bool toggle);
private:
    int bitsize;

    typedef fastarray<char> base;

    using base::operator[];
};


#endif // BITS_H
