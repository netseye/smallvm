/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

// Copyright 2018 John Maloney, Bernat Romagosa, and Jens Mönig

// arrayPrims.cpp - Microblocks arrau primitives
// John Maloney, September 2017

#include <stdio.h>
#include <stdlib.h>

#include "mem.h"
#include "interp.h"

// Helper Functions

static int stringSize(OBJ obj) {
	int wordCount = objWords(obj);
	if (!wordCount) return 0; // empty string
	char *s = (char *) &FIELD(obj, 0);
	int byteCount = 4 * (wordCount - 1);
	for (int i = 0; i < 4; i++) {
		// scan the last word for the null terminator byte
		if (s[byteCount] == 0) break; // found terminator
		byteCount++;
	}
	return byteCount;
}

OBJ primNewArray(OBJ *args) {
	OBJ n = args[0];
	if (!isInt(n) || ((int) n < 0)) return fail(arraySizeError);
	OBJ result = newObj(ArrayClass, obj2int(n), int2obj(0)); // filled with zero integers
	return result;
}

OBJ primNewByteArray(OBJ *args) {
	OBJ n = args[0];
	if (!isInt(n) || ((int) n < 0)) return fail(arraySizeError);
	OBJ result = newObj(ByteArrayClass, (obj2int(n) + 3) / 4, 0); // filled with zero bytes
	return result;
}

OBJ primArrayFill(OBJ *args) {
	OBJ array = args[0];
	OBJ value = args[1];

	if (IS_CLASS(array, ArrayClass)) {
		int end = objWords(array) + HEADER_WORDS;
		for (int i = HEADER_WORDS; i < end; i++) ((OBJ *) array)[i] = value;
	} else if (IS_CLASS(array, ByteArrayClass)) {
		if (!isInt(value)) return fail(byteArrayStoreError);
		uint32 byteValue = obj2int(value);
		if (byteValue > 255) return fail(byteArrayStoreError);
		uint8 *dst = (uint8 *) &FIELD(array, 0);
		uint8 *end = dst + (4 * objWords(array));
		while (dst < end) *dst++ = byteValue;
	} else {
		fail(needsArrayError);
	}
	return falseObj;
}

OBJ primArrayAt(OBJ *args) {
	if (!isInt(args[0])) return fail(needsIntegerIndexError);
	int i = obj2int(args[0]);
	OBJ array = args[1];

	if (IS_CLASS(array, ArrayClass)) {
		if ((i < 1) || (i > objWords(array))) return fail(indexOutOfRangeError);
		return FIELD(array, (i - 1));
	} else if (IS_CLASS(array, ByteArrayClass) || IS_CLASS(array, StringClass)) {
		int byteCount = 4 * objWords(array);
		if IS_CLASS(array, StringClass) byteCount = stringSize(array);
		if ((i < 1) || (i > byteCount)) return fail(indexOutOfRangeError);
		uint8 *bytes = (uint8 *) &FIELD(array, 0);
		return int2obj(bytes[i - 1]);
	}
	return fail(needsArrayError);
}

OBJ primArrayAtPut(OBJ *args) {
	if (!isInt(args[0])) return fail(needsIntegerIndexError);
	int i = obj2int(args[0]);
	OBJ array = args[1];
	OBJ value = args[2];

	if (IS_CLASS(array, ArrayClass)) {
		if ((i < 1) || (i > objWords(array))) return fail(indexOutOfRangeError);
		FIELD(array, (i - 1)) = value;
	} else if (IS_CLASS(array, ByteArrayClass)) {
		if ((i < 1) || (i > (objWords(array) * 4))) return fail(indexOutOfRangeError);
		if (!isInt(value)) return fail(byteArrayStoreError);
		uint32 byteValue = obj2int(value);
		if (byteValue > 255) return fail(byteArrayStoreError);
		((uint8 *) &FIELD(array, 0))[i - 1] = byteValue;
	} else return fail(needsArrayError);
	return falseObj;
}

OBJ primArraySize(OBJ *args) {
	OBJ obj = args[0];

	if (IS_CLASS(obj, ArrayClass)) {
		return int2obj(objWords(obj));
	} else if (IS_CLASS(obj, ByteArrayClass)) {
		return int2obj(4 * objWords(obj));
	} else if (IS_CLASS(obj, StringClass)) {
		return int2obj(stringSize(obj));
	}
	return fail(needsArrayError);
}

OBJ primHexToInt(OBJ *args) {
	if (!IS_CLASS(args[0], StringClass)) return fail(needsStringError);

	char *s = obj2str(args[0]);
	if ('#' == *s) s++; // skip leading # if there is one
	long result = strtol(s, NULL, 16);
	if ((result < -536870912) || (result > 536870911)) return fail(hexRangeError);
	return int2obj(result);
}

OBJ primPeek(OBJ *args) {
	if (!isInt(args[0]) || !isInt(args[1])) return fail(needsIntegerError);
	int *addr = (int *) (((obj2int(args[0]) & 0xF) << 28) | (obj2int(args[1]) & 0xFFFFFFC));
	return int2obj(*addr);
}

OBJ primPoke(OBJ *args) {
	if (!isInt(args[0]) || !isInt(args[1]) || !isInt(args[2])) return fail(needsIntegerError);
	int *addr = (int *) (((obj2int(args[0]) & 0xF) << 28) | (obj2int(args[1]) & 0xFFFFFFC));
	*addr = obj2int(args[2]);
	return falseObj;
}
