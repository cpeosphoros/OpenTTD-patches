/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file saveload_data.h Saveload data declarations. */

#ifndef SAVELOAD_DATA_H
#define SAVELOAD_DATA_H

#include <list>

#include "../core/bitmath_func.hpp"

/** Types of save games. */
enum SavegameType {
	SGT_TTO,    ///< TTO savegame
	SGT_TTD,    ///< TTD  savegame (can be detected incorrectly)
	SGT_TTDP1,  ///< TTDP savegame ( -//- ) (data at NW border)
	SGT_TTDP2,  ///< TTDP savegame in new format (data at SE border)
	SGT_OTTD,   ///< OTTD savegame
	SGT_FTTD,   ///< FTTD savegame
	SGT_INVALID = 0xFF, ///< broken savegame (used internally)
};

/** Type and version of a savegame. */
struct SavegameTypeVersion {
	SavegameType type;  ///< type of savegame
	union {
		struct {
			uint version; ///< version of TTDP savegame (if applicable)
		} ttdp;
		struct {
			uint version;       ///< the major savegame version
			uint minor_version; ///< the minor savegame version
		} ottd;
		struct {
			uint version; ///< savegame version
		} fttd;
	};
};

/**
 * Checks whether the savegame version is older than a given version.
 * @param stv Savegame version to check.
 * @param version Version to check against.
 * @param major Major number of the version to check against, for legacy savegames.
 * @param minor Minor number of the version to check against, ignored if 0, for legacy savegames.
 * @return Savegame version is earlier than the specified version.
 */
static inline bool IsFullSavegameVersionBefore(const SavegameTypeVersion *stv, uint version, uint major = UINT_MAX, uint minor = 0)
{
	switch (stv->type) {
		default: return major > 0;
		case SGT_OTTD: return stv->ottd.version < major || (minor > 0 && stv->ottd.version == major && stv->ottd.minor_version < minor);
		case SGT_FTTD: return stv->fttd.version < version;
	}
}

/**
 * Checks whether the savegame version is legacy and older than a given version.
 * @param stv Savegame version to check.
 * @param major Major number of the version to check against.
 * @param minor Minor number of the version to check against, ignored if 0.
 * @return Savegame version is earlier than the specified version.
 */
static inline bool IsOTTDSavegameVersionBefore(const SavegameTypeVersion *stv, uint16 major, byte minor = 0)
{
	return IsFullSavegameVersionBefore(stv, 0, major, minor);
}

/**
 * Checks whether a given savegame version is the current savegame version.
 * @param stv Savegame version to check.
 */
static inline bool IsCurrentSavegameVersion (const SavegameTypeVersion *stv)
{
	extern const uint16 SAVEGAME_VERSION;
	return (stv->type == SGT_FTTD) && (stv->fttd.version == SAVEGAME_VERSION);
}


/**
 * Constructor disambiguation helper struct.
 *
 * Constructor templates cannot be explicitly selected on construction.
 * If you have, say,
 *     struct S {
 *         template <uint N>
 *         S (...);
 *     };
 * then you cannot say
 *     S<3> s (...);
 * because that is interpreted as if S were a class template, which it is
 * not, and not to choose a specialisation of the constructor. This means
 * that the only template parameters that are useful in a constructor are
 * those that can be deduced automatically. This is easy when the template
 * parameter is a type, but not so when it is an integral value.
 *
 * The following struct helps define template constructors that depend on
 * a value, and not on a type, by turning values into types. Instead of
 * the above, you now write
 *     struct S {
 *         template <uint N>
 *         S (const CDIS<uint>::VAL<N> *, ...);
 *     };
 *     S s (CDIS<uint>::VAL<3>::null(), ...);
 * and everything works, because now the template argument can be deduced
 * (at the cost of extra typing).
 *
 * This trick can also be used when there are no templates but you want a
 * set of overloaded functions that cannot be told apart just by their
 * signature.
 */
template <typename T>
struct CDIS {
	static CONSTEXPR const CDIS *null (void)
	{
		return (const CDIS*) NULL;
	}

	template <T t>
	struct VAL {
		static CONSTEXPR const VAL *null (void)
		{
			return (const VAL*) NULL;
		}
	};
};


/** Type of data saved. */
enum SaveLoadTypes {
	SL_VAR,       ///< Save/load a variable
	SL_REF,       ///< Save/load a reference
	SL_ARR,       ///< Save/load an array
	SL_STR,       ///< Save/load a string
	SL_LST,       ///< Save/load a list
	SL_NULL,      ///< Skip over bytes in the savegame
	/* non-normal save-load types */
	SL_WRITEBYTE, ///< Save/load a constant byte
	SL_INCLUDE,   ///< Include another SaveLoad description
	SL_END        ///< SaveLoad chunk end marker
};

typedef byte SaveLoadType; ///< Save/load type. @see SaveLoadTypes

/**
 * VarTypes is the general bitmasked magic type that tells us
 * certain characteristics about the variable it refers to. For example
 * SLE_FILE_* gives the size(type) as it would be in the savegame and
 * SLE_VAR_* the size(type) as it is in memory during runtime. These are
 * the first 8 bits (0-3 SLE_FILE, 4-7 SLE_VAR).
 * Bits 8-15 are reserved for various flags as explained below
 */
enum VarTypes {
	/* 4 bits allocated for a maximum of 16 types for NumberType */
	SLE_FILE_I8       = 0,
	SLE_FILE_U8       = 1,
	SLE_FILE_I16      = 2,
	SLE_FILE_U16      = 3,
	SLE_FILE_I32      = 4,
	SLE_FILE_U32      = 5,
	SLE_FILE_I64      = 6,
	SLE_FILE_U64      = 7,
	SLE_FILE_STRINGID = 8, ///< StringID offset into strings-array
	/* 7 more possible file-primitives */

	/* 4 bits allocated for a maximum of 16 types for NumberType */
	SLE_VAR_BL    =  0 << 4,
	SLE_VAR_I8    =  1 << 4,
	SLE_VAR_U8    =  2 << 4,
	SLE_VAR_I16   =  3 << 4,
	SLE_VAR_U16   =  4 << 4,
	SLE_VAR_I32   =  5 << 4,
	SLE_VAR_U32   =  6 << 4,
	SLE_VAR_I64   =  7 << 4,
	SLE_VAR_U64   =  8 << 4,
	SLE_VAR_NAME  = 10 << 4, ///< old custom name to be converted to a char pointer
	/* 6 more possible memory-primitives */

	/* Shortcut values */
	SLE_VAR_CHAR = SLE_VAR_I8,

	/* Default combinations of variables. As savegames change, so can variables
	 * and thus it is possible that the saved value and internal size do not
	 * match and you need to specify custom combo. The defaults are listed here */
	SLE_BOOL         = SLE_FILE_I8  | SLE_VAR_BL,
	SLE_INT8         = SLE_FILE_I8  | SLE_VAR_I8,
	SLE_UINT8        = SLE_FILE_U8  | SLE_VAR_U8,
	SLE_INT16        = SLE_FILE_I16 | SLE_VAR_I16,
	SLE_UINT16       = SLE_FILE_U16 | SLE_VAR_U16,
	SLE_INT32        = SLE_FILE_I32 | SLE_VAR_I32,
	SLE_UINT32       = SLE_FILE_U32 | SLE_VAR_U32,
	SLE_INT64        = SLE_FILE_I64 | SLE_VAR_I64,
	SLE_UINT64       = SLE_FILE_U64 | SLE_VAR_U64,
	SLE_CHAR         = SLE_FILE_I8  | SLE_VAR_CHAR,
	SLE_STRINGID     = SLE_FILE_STRINGID | SLE_VAR_U16,
	SLE_NAME         = SLE_FILE_STRINGID | SLE_VAR_NAME,

	/* Shortcut values */
	SLE_UINT  = SLE_UINT32,
	SLE_INT   = SLE_INT32,
};

typedef byte VarType;

/**
 * Get the NumberType of a setting. This describes the integer type
 * as it is represented in memory
 * @param type VarType holding information about the variable-type
 * @return return the SLE_VAR_* part of a variable-type description
 */
static inline VarType GetVarMemType(VarType type)
{
	return type & 0xF0; // GB(type, 4, 4) << 4;
}

/**
 * Get the #FileType of a setting. This describes the integer type
 * as it is represented in a savegame/file
 * @param type VarType holding information about the file-type
 * @param return the SLE_FILE_* part of a variable-type description
 */
static inline VarType GetVarFileType(VarType type)
{
	return type & 0xF; // GB(type, 0, 4);
}

/**
 * Check if the given saveload type is a numeric type.
 * @param conv the type to check
 * @return True if it's a numeric type.
 */
static inline bool IsNumericType(VarType conv)
{
	return GetVarMemType(conv) <= SLE_VAR_U64;
}

/**
 * Return the size in bytes of a certain type of normal/atomic variable
 * as it appears in memory. See VarTypes
 * @param conv VarType type of variable that is used for calculating the size
 * @return Return the size of this type in bytes
 */
static inline uint SlCalcConvMemLen(VarType conv)
{
	assert(IsNumericType(conv));

	extern const byte _conv_mem_size[];
	byte length = GB(conv, 4, 4);
	return _conv_mem_size[length];
}

/**
 * Return the size in bytes of a certain type of normal/atomic variable
 * as it appears in a saved game. See VarTypes
 * @param conv VarType type of variable that is used for calculating the size
 * @return Return the size of this type in bytes
 */
static inline byte SlCalcConvFileLen(VarType conv)
{
	extern const byte _conv_file_size[];
	byte length = GB(conv, 0, 4);
	assert(length <= SLE_FILE_STRINGID);
	return _conv_file_size[length];
}

int64 ReadValue(const void *ptr, VarType conv);
void WriteValue(void *ptr, VarType conv, int64 val);


/**
 * StrTypes encodes information about saving and loading of strings (#SLE_STR).
 */
enum StrTypes {
	SLS_NONE          = 0,
	SLS_QUOTED        = 1 << 0, ///< string is enclosed in quotes
	SLS_ALLOW_CONTROL = 1 << 1, ///< allow control codes in the string
	SLS_ALLOW_NEWLINE = 1 << 2, ///< allow newlines in the string
	/* 5 more possible flags */
};

typedef byte StrType;

/** Type of reference (#SLE_REF, #SLE_CONDREF). */
enum SLRefType {
	REF_ORDER          =  0, ///< Load/save a reference to an order.
	REF_VEHICLE        =  1, ///< Load/save a reference to a vehicle.
	REF_STATION        =  2, ///< Load/save a reference to a station.
	REF_TOWN           =  3, ///< Load/save a reference to a town.
	REF_VEHICLE_OLD    =  4, ///< Load/save an old-style reference to a vehicle (for pre-4.4 savegames).
	REF_ROADSTOPS      =  5, ///< Load/save a reference to a bus/truck stop.
	REF_DOCKS          =  6, ///< Load/save a reference to a dock.
	REF_ENGINE_RENEWS  =  7, ///< Load/save a reference to an engine renewal (autoreplace).
	REF_CARGO_PACKET   =  8, ///< Load/save a reference to a cargo packet.
	REF_ORDERLIST      =  9, ///< Load/save a reference to an orderlist.
	REF_STORAGE        = 10, ///< Load/save a reference to a persistent storage.
	REF_LINK_GRAPH     = 11, ///< Load/save a reference to a link graph.
	REF_LINK_GRAPH_JOB = 12, ///< Load/save a reference to a link graph job.
};


/** Flags directing saving/loading of a variable */
enum SaveLoadFlags {
	SLF_GLOBAL          = 1 << 0, ///< global variable, instead of a struct field
	SLF_NOT_IN_SAVE     = 1 << 1, ///< do not save with savegame, basically client-based
	SLF_NOT_IN_CONFIG   = 1 << 2, ///< do not save to config file
	SLF_NO_NETWORK_SYNC = 1 << 3, ///< do not synchronize over network (but it is saved if SLF_NOT_IN_SAVE is not set)
};

/** SaveLoad type struct. Do NOT use this directly but use the SLE_ macros defined just below! */
struct SaveLoad {
	struct VersionRange {
		uint16 from; ///< save/load the variable starting from this savegame version
		uint16 to;   ///< save/load the variable until this savegame version

		CONSTEXPR VersionRange (void) : from(0), to(UINT16_MAX) { }

		CONSTEXPR VersionRange (uint16 from, uint16 to) : from(from), to(to) { }
	};

	SaveLoadType type;     ///< object type
	byte conv;             ///< object subtype/conversion
	byte flags;            ///< save/load flags
	uint16 length;         ///< (conditional) length of the variable (eg. arrays) (max array size is 65536 elements)
	VersionRange version;  ///< save/load the variable in this version range
	VersionRange legacy;   ///< save/load the variable in this legacy version range
	/* NOTE: This element either denotes the address of the variable for a global
	 * variable, or the offset within a struct which is then bound to a variable
	 * during runtime. Decision on which one to use is controlled by the function
	 * that is called to save it. address: global=true, offset: global=false.
	 * For SL_INCLUDE, this points to the SaveLoad object to be included. */
	void *address;         ///< address of variable OR offset of variable in the struct (max offset is 65536)

	CONSTEXPR SaveLoad (void)
		: type(SL_END), conv(0), flags(0), length(0),
		  version(), legacy(), address(NULL)
	{
	}

	CONSTEXPR SaveLoad (const SaveLoad *include)
		: type(SL_INCLUDE), conv(0), flags(0), length(0),
		  version(), legacy(), address(const_cast<SaveLoad *>(include))
	{
	}

	/** Address constructor for a global variable. */
	static inline void *saveload_address (void *address)
	{
		return address;
	}

	/** Address constructor for a struct variable. */
	static inline void *saveload_address (size_t offset)
	{
		return (void*) offset;
	}

	/* Validate a struct/vartype pair. */

#define DEFINE_VALIDATE_CONV(conv,size) \
	template <typename T>                                           \
	static inline CONSTEXPR VarTypes validate_conv (const T *,      \
		const CDIS<VarTypes>::VAL<conv> *, VarTypes c)          \
	{                                                               \
		assert_tcompile (sizeof(T) == size);                    \
		return c;                                               \
	}

	DEFINE_VALIDATE_CONV (SLE_VAR_BL,  1)
	DEFINE_VALIDATE_CONV (SLE_VAR_I8,  1)
	DEFINE_VALIDATE_CONV (SLE_VAR_U8,  1)
	DEFINE_VALIDATE_CONV (SLE_VAR_I16, 2)
	DEFINE_VALIDATE_CONV (SLE_VAR_U16, 2)
	DEFINE_VALIDATE_CONV (SLE_VAR_I32, 4)
	DEFINE_VALIDATE_CONV (SLE_VAR_U32, 4)
	DEFINE_VALIDATE_CONV (SLE_VAR_I64, 8)
	DEFINE_VALIDATE_CONV (SLE_VAR_U64, 8)

#undef DEFINE_VALIDATE_CONV

	static inline CONSTEXPR VarTypes validate_conv (char *const *,
		const CDIS<VarTypes>::VAL<SLE_VAR_NAME> *, VarTypes conv)
	{
		return conv;
	}

	/* Validate array parameters. */

#define DEFINE_VALIDATE_ARRAY(conv,size)                                \
	template <size_t ALLOC, uint LENGTH>                            \
	static inline CONSTEXPR VarTypes validate_array (VarTypes c,    \
		const CDIS<VarTypes>::VAL<conv> *)                      \
	{                                                               \
		assert_tcompile (size * LENGTH <= ALLOC);               \
		return c;                                               \
	}

	DEFINE_VALIDATE_ARRAY (SLE_VAR_BL,  1)
	DEFINE_VALIDATE_ARRAY (SLE_VAR_I8,  1)
	DEFINE_VALIDATE_ARRAY (SLE_VAR_U8,  1)
	DEFINE_VALIDATE_ARRAY (SLE_VAR_I16, 2)
	DEFINE_VALIDATE_ARRAY (SLE_VAR_U16, 2)
	DEFINE_VALIDATE_ARRAY (SLE_VAR_I32, 4)
	DEFINE_VALIDATE_ARRAY (SLE_VAR_U32, 4)
	DEFINE_VALIDATE_ARRAY (SLE_VAR_I64, 8)
	DEFINE_VALIDATE_ARRAY (SLE_VAR_U64, 8)

#undef DEFINE_VALIDATE_ARRAY

	template <typename T, VarTypes CONV, uint LENGTH>
	static inline CONSTEXPR VarTypes validate_array (const T *,
		const CDIS<VarTypes>::VAL<CONV> *conv,
		const CDIS<uint>::VAL<LENGTH> *l, VarTypes c)
	{
		return validate_array <sizeof(T), LENGTH> (c, conv);
	}

	template <typename T, uint N, VarTypes CONV, uint LENGTH>
	static inline CONSTEXPR VarTypes validate_array (const T (*) [N],
		const CDIS<VarTypes>::VAL<CONV> *conv,
		const CDIS<uint>::VAL<LENGTH> *l, VarTypes c)
	{
		return validate_array <sizeof(T) * N, LENGTH> (c, conv);
	}

	/* Validate a struct/reftype pair. */

#define DEFINE_VALIDATE_REF(T,r) \
	static inline CONSTEXPR SLRefType validate_reftype (const T*,   \
		const CDIS<SLRefType>::VAL<r> *)                        \
	{ return r; }

	DEFINE_VALIDATE_REF (struct Order,             REF_ORDER)
	DEFINE_VALIDATE_REF (struct Vehicle,           REF_VEHICLE)
	DEFINE_VALIDATE_REF (struct Station,           REF_STATION)
	DEFINE_VALIDATE_REF (struct Town,              REF_TOWN)
	DEFINE_VALIDATE_REF (struct Vehicle,           REF_VEHICLE_OLD)
	DEFINE_VALIDATE_REF (struct RoadStop,          REF_ROADSTOPS)
	DEFINE_VALIDATE_REF (struct Dock,              REF_DOCKS)
	DEFINE_VALIDATE_REF (struct EngineRenew,       REF_ENGINE_RENEWS)
	DEFINE_VALIDATE_REF (struct CargoPacket,       REF_CARGO_PACKET)
	DEFINE_VALIDATE_REF (struct OrderList,         REF_ORDERLIST)
	DEFINE_VALIDATE_REF (struct PersistentStorage, REF_STORAGE)
	DEFINE_VALIDATE_REF (class  LinkGraph,         REF_LINK_GRAPH)
	DEFINE_VALIDATE_REF (class  LinkGraphJob,      REF_LINK_GRAPH_JOB)

#undef DEFINE_VALIDATE_REF

	/** Construct a saveload object for a variable. */
	template <typename T, typename ADDR, VarTypes CONV>
	CONSTEXPR SaveLoad (const CDIS<SaveLoadTypes>::VAL<SL_VAR> *, const T *p,
			ADDR address, byte flags,
			const CDIS<VarTypes>::VAL<CONV> *,
			uint16 from, uint16 to, uint16 lfrom, uint16 lto)
		: type(SL_VAR), conv(validate_conv (p, CDIS<VarTypes>::VAL<(VarTypes)(CONV & 0xF0)>::null(), CONV)),
			flags(flags), length(0),
			version (from, to), legacy (lfrom, lto),
			address(saveload_address(address))
	{
	}

	/** Construct a saveload object for a reference. */
	template <typename T, typename ADDR, SLRefType REFTYPE>
	CONSTEXPR SaveLoad (const CDIS<SaveLoadTypes>::VAL<SL_REF> *, T *const *,
			ADDR address, byte flags,
			const CDIS<SLRefType>::VAL<REFTYPE> *reftype,
			uint16 from, uint16 to, uint16 lfrom, uint16 lto)
		: type(SL_REF), conv(validate_reftype ((T*)NULL, reftype)),
			flags(flags), length(0),
			version (from, to), legacy (lfrom, lto),
			address(saveload_address(address))
	{
	}

	/** Construct a saveload object for an array. */
	template <typename T, typename ADDR, VarTypes CONV, uint LENGTH>
	CONSTEXPR SaveLoad (const CDIS<SaveLoadTypes>::VAL<SL_ARR> *, const T *p,
			ADDR address, byte flags,
			const CDIS<VarTypes>::VAL<CONV> *,
			const CDIS<uint>::VAL<LENGTH> *l,
			uint16 from, uint16 to, uint16 lfrom, uint16 lto)
		: type(SL_ARR), conv(validate_array (p, CDIS<VarTypes>::VAL<(VarTypes)(CONV & 0xF0)>::null(), l, CONV)),
			flags(flags), length(LENGTH),
			version (from, to), legacy (lfrom, lto),
			address(saveload_address(address))
	{
		assert_tcompile (LENGTH > 0);
		assert_tcompile (LENGTH <= UINT16_MAX);
	}

	/** Construct a saveload object for a fixed-buffer string. */
	template <uint N, typename ADDR>
	CONSTEXPR SaveLoad (const CDIS<SaveLoadTypes>::VAL<SL_STR> *, const char (*) [N],
			ADDR address, byte flags,
			StrTypes conv,
			uint16 from, uint16 to, uint16 lfrom, uint16 lto)
		: type(SL_STR), conv(conv), flags(flags), length(N),
			version (from, to), legacy (lfrom, lto),
			address(saveload_address(address))
	{
		assert_tcompile (N > 0);
		assert_tcompile (N <= UINT16_MAX);
	}

	/** Construct a saveload object for an allocated string. */
	template <typename ADDR>
	CONSTEXPR SaveLoad (const CDIS<SaveLoadTypes>::VAL<SL_STR> *, const char *const *,
			ADDR address, byte flags,
			StrTypes conv,
			uint16 from, uint16 to, uint16 lfrom, uint16 lto)
		: type(SL_STR), conv(conv), flags(flags), length(0),
			version (from, to), legacy (lfrom, lto),
			address(saveload_address(address))
	{
	}

	/** Construct a saveload object for a reference list. */
	template <typename T, typename ADDR, SLRefType REFTYPE>
	CONSTEXPR SaveLoad (const CDIS<SaveLoadTypes>::VAL<SL_LST> *, const std::list<T*> *,
			ADDR address, byte flags,
			const CDIS<SLRefType>::VAL<REFTYPE> *reftype,
			uint16 from, uint16 to, uint16 lfrom, uint16 lto)
		: type(SL_LST), conv(validate_reftype ((T*)NULL, reftype)),
			flags(flags), length(0),
			version (from, to), legacy (lfrom, lto),
			address(saveload_address(address))
	{
	}

	/** Construct a saveload object for a null byte sequence. */
	template <uint LENGTH>
	CONSTEXPR SaveLoad (const CDIS<SaveLoadTypes>::VAL<SL_NULL> *,
			const CDIS<uint>::VAL<LENGTH> *,
			uint16 from, uint16 to, uint16 lfrom, uint16 lto)
		: type(SL_NULL), conv(0), flags(SLF_NOT_IN_CONFIG), length(LENGTH),
			version (from, to), legacy (lfrom, lto),
			address(NULL)
	{
		assert_tcompile (LENGTH > 0);
		assert_tcompile (LENGTH <= UINT16_MAX);
	}

	/** Construct a saveload object for a struct constant byte. */
	template <typename T>
	CONSTEXPR SaveLoad (const CDIS<SaveLoadTypes>::VAL<SL_WRITEBYTE> *, const T *,
			size_t offset, byte conv,
			uint16 from, uint16 to, uint16 lfrom, uint16 lto)
		: type(SL_WRITEBYTE), conv(conv), flags(0), length(0),
			version (from, to), legacy (lfrom, lto),
			address(saveload_address(offset))
	{
		assert_tcompile (sizeof(T) == 1);
	}

	/**
	 * Get the address of the variable that this saveload description
	 * encodes for a given object. If the saveload description encodes
	 * a global object, its address is returned (and object can be NULL).
	 * If the saveload description encodes a struct variable, then the
	 * address of the corresponding field in the given object is returned
	 * (and object cannot be NULL).
	 * @param object The object to which this description applies.
	 * @return The address of the encoded variable in the given object.
	 */
	void *get_variable_address (void *object = NULL) const
	{
		return (this->flags & SLF_GLOBAL) ? this->address :
			((byte*)object + (ptrdiff_t)this->address);
	}

	const void *get_variable_address (const void *object) const
	{
		return this->get_variable_address (const_cast<void *>(object));
	}

	/** Check if this object is valid in a certain savegame version. */
	bool is_valid (const SavegameTypeVersion *stv) const
	{
		switch (stv->type) {
			default:
				if (0 < this->legacy.from) return false;
				break;

			case SGT_OTTD:
				if (stv->ottd.version < this->legacy.from || stv->ottd.version > this->legacy.to) return false;
				break;

			case SGT_FTTD:
				if (stv->fttd.version < this->version.from || stv->fttd.version > this->version.to) return false;
				break;
		}

		if (this->flags & SLF_NOT_IN_SAVE) return false;

		return true;
	}

	/** Check if this object is valid in the current savegame version. */
	bool is_currently_valid (void) const
	{
		extern const uint16 SAVEGAME_VERSION;
		if (SAVEGAME_VERSION < this->version.from || SAVEGAME_VERSION > this->version.to) return false;

		return true;
	}
};

/** Highest possible savegame version. */
#define SL_MAX_VERSION UINT16_MAX

/** Workaround for MSVC broken variadic macro support. */
#define SLE_EXPAND(x) x

/** Substitute first paramenter if non-empty, else second. */
#define SLE_DEFAULT_IF_EMPTY(value, default) ((sizeof(#value) != 1) ? (value + 0) : (default))

/**
 * Generic SaveLoad object, with version range and legacy version range.
 * @param cons     Constructor macro
 * @param pointer  Random pointer to variable type (value will never be used).
 * @param address  Address of variable or offset of variable in the struct
 * @param flags    Save/load flags
 * @param conv     Subtype/conversion of the data between memory and savegame.
 * @param length   Length of object (for arrays and strings)
 * @param from     First savegame version that has the field.
 * @param to       Last savegame version that has the field, empty for maximum possible value.
 * @param lfrom    First legacy savegame version that has the field.
 * @param lto      Last legacy savegame version that has the field, empty for maximum possible value.
 * @note This macro should not be used directly.
 */
#define SLE_ANY_2(cons, pointer, address, flags, conv, length, from, to, lfrom, lto) cons (pointer, address, flags, conv, length, SLE_DEFAULT_IF_EMPTY(from, SL_MAX_VERSION), SLE_DEFAULT_IF_EMPTY(to, SL_MAX_VERSION), lfrom, SLE_DEFAULT_IF_EMPTY(lto, SL_MAX_VERSION))

/**
 * Generic SaveLoad object, with version range.
 * @param cons     Constructor macro
 * @param pointer  Random pointer to variable type (value will never be used).
 * @param address  Address of variable or offset of variable in the struct
 * @param flags    Save/load flags
 * @param conv     Subtype/conversion of the data between memory and savegame.
 * @param length   Length of object (for arrays and strings)
 * @param from     First savegame version that has the field.
 * @param to       Last savegame version that has the field, empty for maximum possible value.
 * @note This macro should not be used directly.
 */
#define SLE_ANY_1(cons, pointer, address, flags, conv, length, from, to, ...) SLE_ANY_2(cons, pointer, address, flags, conv, length, from, to, SL_MAX_VERSION, 0)

/**
 * Generic SaveLoad object, without version range.
 * @param cons     Constructor macro
 * @param pointer  Random pointer to variable type (value will never be used).
 * @param address  Address of variable or offset of variable in the struct
 * @param flags    Save/load flags
 * @param conv     Subtype/conversion of the data between memory and savegame.
 * @param length   Length of object (for arrays and strings)
 * @note This macro should not be used directly.
 */
#define SLE_ANY_0(cons, pointer, address, flags, conv, length, ...) SLE_ANY_2(cons, pointer, address, flags, conv, length, 0, , 0, )

/**
 * Generic SaveLoad object, with or without version range.
 * @param cons     Constructor macro
 * @param pointer  Random pointer to variable type (value will never be used).
 * @param address  Address of variable or offset of variable in the struct
 * @param global   Global flag, or 0.
 * @param flags    Save/load flags (other than the global flag)
 * @param conv     Subtype/conversion of the data between memory and savegame.
 * @param length   Length of object (for arrays and strings)
 * @param from     First savegame version that has the field (optional).
 * @param to       Last savegame version that has the field (optional).
 * @param lfrom    First legacy savegame version that has the field (optional).
 * @param lto      Last legacy savegame version that has the field (optional).
 * @note Both savegame version interval endpoints must be present for any given range.
 * @note There should be a trailing empty argument to this macro.
 * @note This macro should not be used directly.
 */
#define SLE_ANY_(cons, pointer, address, global, flags, ...) \
    SLE_EXPAND(SLE_ANY__(cons, pointer, address, global, flags, __VA_ARGS__ 2, INVALID, 1, INVALID, 0, INVALID, ))
#define SLE_ANY__(cons, pointer, address, global, flags, conv, length, from, to, lfrom, lto, n, ...) \
    SLE_ANY_##n (cons, pointer, address, (global) | (flags), conv, length, from, to, lfrom, lto)

/** Struct variable pointer-address-global helper. */
#define SLE_HELPER(callback, base, variable, ...) SLE_EXPAND (callback (cpp_pointer(base, variable), cpp_offsetof(base, variable), 0, __VA_ARGS__, ))

/** Global variable pointer-address-global helper. */
#define SLEG_HELPER(callback, variable, ...) SLE_EXPAND (callback (&variable, &variable, SLF_GLOBAL, __VA_ARGS__, ))


/** Variable construction callback macros. */
#define SLC_VAR(pointer, address, flags, conv, length, ...) \
	SaveLoad (CDIS<SaveLoadTypes>::VAL<SL_VAR>::null(), pointer, address, flags, CDIS<VarTypes>::VAL<(VarTypes)(conv)>::null(), __VA_ARGS__)
#define SLEX_VAR_(pointer, address, global, flags, conv, ...) \
	SLE_EXPAND (SLE_ANY_ (SLC_VAR, pointer, address, global, flags, conv, UNUSED, __VA_ARGS__))

/**
 * Storage of a variable.
 * @param base     Name of the class or struct containing the variable (SLE and SLEF).
 * @param variable Name of the variable in the class or struct referenced by \a base (SLE and SLEF) or global variable (SLEG and SLEGF).
 * @param flag     Additional flags (SLEF and SLEGF).
 * @param conv     Storage of the data in memory and in the savegame.
 * @param from     First savegame version that has the field (optional).
 * @param to       Last savegame version that has the field (optional).
 * @param lfrom    First legacy savegame version that has the field (optional).
 * @param lto      Last legacy savegame version that has the field (optional).
 * @note Both savegame version interval endpoints must be present for any given range.
 */
#define SLEF_VAR(...)  SLE_EXPAND (SLE_HELPER  (SLEX_VAR_, __VA_ARGS__))
#define SLEGF_VAR(...) SLE_EXPAND (SLEG_HELPER (SLEX_VAR_, __VA_ARGS__))
#define SLE_VAR(base, variable, ...) SLEF_VAR(base, variable, 0, __VA_ARGS__)
#define SLEG_VAR(variable, ...) SLEGF_VAR(variable, 0, __VA_ARGS__)

/** Reference construction callback macros. */
#define SLC_REF(pointer, address, flags, reftype, length, ...) \
	SaveLoad (CDIS<SaveLoadTypes>::VAL<SL_REF>::null(), pointer, address, flags, CDIS<SLRefType>::VAL<reftype>::null(), __VA_ARGS__)
#define SLEX_REF_(pointer, address, global, flags, reftype, ...) \
	SLE_EXPAND (SLE_ANY_ (SLC_REF, pointer, address, global, flags, reftype, UNUSED, __VA_ARGS__))

/**
 * Storage of a reference.
 * @param base     Name of the class or struct containing the reference (SLE and SLEF).
 * @param variable Name of the variable in the class or struct referenced by \a base (SLE and SLEF) or global variable (SLEG and SLEGF).
 * @param flag     Additional flags (SLEF and SLEGF).
 * @param reftype  Type of the reference, a value from #SLRefType.
 * @param from     First savegame version that has the field (optional).
 * @param to       Last savegame version that has the field (optional).
 * @param lfrom    First legacy savegame version that has the field (optional).
 * @param lto      Last legacy savegame version that has the field (optional).
 * @note Both savegame version interval endpoints must be present for any given range.
 */
#define SLEF_REF(...)  SLE_EXPAND (SLE_HELPER  (SLEX_REF_, __VA_ARGS__))
#define SLEGF_REF(...) SLE_EXPAND (SLEG_HELPER (SLEX_REF_, __VA_ARGS__))
#define SLE_REF(base, variable, ...) SLEF_REF(base, variable, 0, __VA_ARGS__)
#define SLEG_REF(variable, ...) SLEGF_REF(variable, 0, __VA_ARGS__)

/** Array construction callback macros. */
#define SLC_ARR(pointer, address, flags, conv, length, ...) \
	SaveLoad (CDIS<SaveLoadTypes>::VAL<SL_ARR>::null(), pointer, address, flags, CDIS<VarTypes>::VAL<(VarTypes)(conv)>::null(), CDIS<uint>::VAL<length>::null(), __VA_ARGS__)
#define SLEX_ARR_(pointer, address, global, flags, conv, length, ...) \
	SLE_EXPAND (SLE_ANY_ (SLC_ARR, pointer, address, global, flags, conv, length, __VA_ARGS__))

/**
 * Storage of an array.
 * @param base     Name of the class or struct containing the array (SLE and SLEF).
 * @param variable Name of the variable in the class or struct referenced by \a base (SLE and SLEF) or global variable (SLEG and SLEGF).
 * @param flag     Additional flags (SLEF and SLEGF).
 * @param conv     Storage of the data in memory and in the savegame.
 * @param length   Number of elements in the array.
 * @param from     First savegame version that has the array (optional).
 * @param to       Last savegame version that has the array (optional).
 * @param lfrom    First legacy savegame version that has the array (optional).
 * @param lto      Last legacy savegame version that has the array (optional).
 * @note Both savegame version interval endpoints must be present for any given range.
 */
#define SLEF_ARR(...)  SLE_EXPAND (SLE_HELPER  (SLEX_ARR_, __VA_ARGS__))
#define SLEGF_ARR(...) SLE_EXPAND (SLEG_HELPER (SLEX_ARR_, __VA_ARGS__))
#define SLE_ARR(base, variable, ...) SLEF_ARR(base, variable, 0, __VA_ARGS__)
#define SLEG_ARR(variable, ...) SLEGF_ARR(variable, 0, __VA_ARGS__)

/** String construction callback macros. */
#define SLC_STR(pointer, address, flags, conv, length, ...) \
	SaveLoad (CDIS<SaveLoadTypes>::VAL<SL_STR>::null(), pointer, address, flags, (StrTypes)(conv), __VA_ARGS__)
#define SLEX_STR_(pointer, address, global, flags, conv, ...) \
	SLE_EXPAND (SLE_ANY_ (SLC_STR, pointer, address, global, flags, conv, UNUSED, __VA_ARGS__))

/**
 * Storage of a string.
 * @param base     Name of the class or struct containing the string (SLE and SLEF).
 * @param variable Name of the variable in the class or struct referenced by \a base (SLE and SLEF) or global variable (SLEG and SLEGF).
 * @param flag     Additional flags (SLEF and SLEGF).
 * @param conv     Storage of the data in memory and in the savegame.
 * @param length   Number of elements in the string (only used for fixed size buffers).
 * @param from     First savegame version that has the string (optional).
 * @param to       Last savegame version that has the string (optional).
 * @param lfrom    First legacy savegame version that has the string (optional).
 * @param lto      Last legacy savegame version that has the string (optional).
 * @note Both savegame version interval endpoints must be present for any given range.
 */
#define SLEF_STR(...)  SLE_EXPAND (SLE_HELPER  (SLEX_STR_, __VA_ARGS__))
#define SLEGF_STR(...) SLE_EXPAND (SLEG_HELPER (SLEX_STR_, __VA_ARGS__))
#define SLE_STR(base, variable, ...) SLEF_STR(base, variable, 0, __VA_ARGS__)
#define SLEG_STR(variable, ...) SLEGF_STR(variable, 0, __VA_ARGS__)

/** List construction callback macros. */
#define SLC_LST(pointer, address, flags, reftype, length, ...) \
	SaveLoad (CDIS<SaveLoadTypes>::VAL<SL_LST>::null(), pointer, address, flags, CDIS<SLRefType>::VAL<reftype>::null(), __VA_ARGS__)
#define SLEX_LST_(pointer, address, global, flags, reftype, ...) \
	SLE_EXPAND (SLE_ANY_ (SLC_LST, pointer, address, global, flags, reftype, UNUSED, __VA_ARGS__))

/**
 * Storage of a list.
 * @param base     Name of the class or struct containing the list (SLE and SLEF).
 * @param variable Name of the variable in the class or struct referenced by \a base (SLE and SLEF) or global variable (SLEG and SLEGF).
 * @param flag     Additional flags (SLEF and SLEGF).
 * @param reftype  Type of the reference, a value from #SLRefType.
 * @param from     First savegame version that has the list (optional).
 * @param to       Last savegame version that has the list (optional).
 * @param lfrom    First legacy savegame version that has the list (optional).
 * @param lto      Last legacy savegame version that has the list (optional).
 * @note Both savegame version interval endpoints must be present for any given range.
 */
#define SLEF_LST(...)  SLE_EXPAND (SLE_HELPER  (SLEX_LST_, __VA_ARGS__))
#define SLEGF_LST(...) SLE_EXPAND (SLEG_HELPER (SLEX_LST_, __VA_ARGS__))
#define SLE_LST(base, variable, ...) SLEF_LST(base, variable, 0, __VA_ARGS__)
#define SLEG_LST(variable, ...) SLEGF_LST(variable, 0, __VA_ARGS__)

/** Empty space construction callback macro. */
#define SLC_NULL(pointer, address, flags, conv, length, ...) \
	SaveLoad (CDIS<SaveLoadTypes>::VAL<SL_NULL>::null(), CDIS<uint>::VAL<length>::null(), __VA_ARGS__)

/**
 * Empty space.
 * @param length Length of the empty space.
 * @param from   First savegame version that has the empty space (optional).
 * @param to     Last savegame version that has the empty space (optional).
 * @param lfrom  First legacy savegame version that has the empty space (optional).
 * @param lto    Last legacy savegame version that has the empty space (optional).
 * @note Both savegame version interval endpoints must be present for any given range.
 */
#define SLE_NULL(...) SLE_EXPAND(SLE_NULL_(__VA_ARGS__, ))
#define SLE_NULL_(length, ...) SLE_EXPAND(SLE_ANY_(SLC_NULL, UNUSED, UNUSED, UNUSED, UNUSED, UNUSED, length, __VA_ARGS__))

/** Translate values ingame to different values in the savegame and vv. */
#define SLE_WRITEBYTE(base, variable, value) \
	SLE_ANY_ (SLC_WRITEBYTE, cpp_pointer(base, variable), cpp_offsetof(base, variable), UNUSED, UNUSED, value, UNUSED, )
#define SLC_WRITEBYTE(pointer, address, flags, conv, length, ...) \
	SaveLoad (CDIS<SaveLoadTypes>::VAL<SL_WRITEBYTE>::null(), pointer, address, conv, __VA_ARGS__)

/** Include another SaveLoad object. */
#define SLE_INCLUDE(include) SaveLoad(include)

/** End marker of a struct/class save or load. */
#define SLE_END() SaveLoad()


size_t SlCalcObjLength(const void *object, const SaveLoad *sld);

void SlObjectPtrs(void *object, const SaveLoad *sld, const SavegameTypeVersion *stv);

#endif /* SAVELOAD_DATA_H */
