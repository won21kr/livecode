/* Copyright (C) 2003-2014 Runtime Revolution Ltd.
 
 This file is part of LiveCode.
 
 LiveCode is free software; you can redistribute it and/or modify it under
 the terms of the GNU General Public License v3 as published by the Free
 Software Foundation.
 
 LiveCode is distributed in the hope that it will be useful, but WITHOUT ANY
 WARRANTY; without even the implied warranty of MERCHANTABILITY or
 FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 for more details.
 
 You should have received a copy of the GNU General Public License
 along with LiveCode.  If not see <http://www.gnu.org/licenses/>.  */

////////////////////////////////////////////////////////////////////////////////

#include "prefix.h"

#include "module-canvas.h"
#include "module-canvas-internal.h"

////////////////////////////////////////////////////////////////////////////////

// Useful stuff

bool MCMemoryAllocateArrayCopy(const void *p_array, uint32_t p_size, uint32_t p_count, void *&r_copy)
{
	return MCMemoryAllocateCopy(p_array, p_size * p_count, r_copy);
}

template <class T>
bool MCMemoryAllocateArrayCopy(const T *p_array, uint32_t p_count, T *&r_copy)
{
	void *t_copy;
	if (!MCMemoryAllocateArrayCopy(p_array, sizeof(T), p_count, t_copy))
		return false;
	
	r_copy = (T*)t_copy;
	return true;
}

bool MCArrayStoreReal(MCArrayRef p_array, MCNameRef p_key, real64_t p_value)
{
	bool t_success;
	t_success = true;
	
	MCNumberRef t_number;
	t_number = nil;
	
	if (t_success)
		t_success = MCNumberCreateWithReal(p_value, t_number);
	
	if (t_success)
		t_success = MCArrayStoreValue(p_array, false, p_key, t_number);
	
	MCValueRelease(t_number);
	
	return t_success;
}

bool MCArrayFetchReal(MCArrayRef p_array, MCNameRef p_key, real64_t &r_value)
{
	bool t_success;
	t_success = true;
	
	MCValueRef t_value;
	t_value = nil;
	
	if (t_success)
		t_success = MCArrayFetchValue(p_array, false, p_key, t_value);
	
	if (t_success)
	{
		t_success = MCValueGetTypeCode(t_value) == kMCValueTypeCodeNumber;
		// TODO - throw error on failure
	}
	
	if (t_success)
		r_value = MCNumberFetchAsReal((MCNumberRef)t_value);
	
	return t_success;
}

bool MCArrayFetchString(MCArrayRef p_array, MCNameRef p_key, MCStringRef &r_value)
{
	bool t_success;
	t_success = true;
	
	MCValueRef t_value;
	t_value = nil;
	
	if (t_success)
		t_success = MCArrayFetchValue(p_array, false, p_key, t_value);
	
	if (t_success)
	{
		if (MCValueGetTypeCode(t_value) == kMCValueTypeCodeName)
			r_value = MCNameGetString((MCNameRef)t_value);
		else if (MCValueGetTypeCode(t_value) == kMCValueTypeCodeString)
			r_value = (MCStringRef)t_value;
		else
			t_success = false;
	}
	
	return t_success;
}

bool MCArrayFetchCanvasColor(MCArrayRef p_array, MCNameRef p_key, MCCanvasColorRef &r_value)
{
	bool t_success;
	t_success = true;
	
	MCValueRef t_value;
	t_value = nil;
	
	if (t_success)
		t_success = MCArrayFetchValue(p_array, false, p_key, t_value);
	
	if (t_success)
		t_success = MCValueGetTypeInfo(t_value) == kMCCanvasColorTypeInfo;
	
	if (t_success)
		r_value = (MCCanvasColorRef)t_value;
	
	return t_success;
}

//////////

bool MCProperListFetchNumberAtIndex(MCProperListRef p_list, uindex_t p_index, MCNumberRef &r_number)
{
	if (p_index >= MCProperListGetLength(p_list))
		return false;
		
	MCValueRef t_value;
	t_value = MCProperListFetchElementAtIndex(p_list, p_index);
	if (t_value == nil)
		return false;
	
	if (MCValueGetTypeInfo(t_value) != kMCNumberTypeInfo)
		return false;
	
	r_number = (MCNumberRef)t_value;
	
	return true;
}

bool MCProperListFetchRealAtIndex(MCProperListRef p_list, uindex_t p_index, real64_t &r_real)
{
	MCNumberRef t_number;
	if (!MCProperListFetchNumberAtIndex(p_list, p_index, t_number))
		return false;
	
	r_real = MCNumberFetchAsReal(t_number);
	
	return true;
}

bool MCProperListFetchIntegerAtIndex(MCProperListRef p_list, uindex_t p_index, integer_t &r_integer)
{
	MCNumberRef t_number;
	if (!MCProperListFetchNumberAtIndex(p_list, p_index, t_number))
		return false;

	r_integer = MCNumberFetchAsInteger(t_number);
	
	return true;
}

bool MCProperListFetchAsArrayOfReal(MCProperListRef p_list, uindex_t p_size, real64_t *r_reals)
{
	if (p_size != MCProperListGetLength(p_list))
		return false;
	
	for (uindex_t i = 0; i < p_size; i++)
		if (!MCProperListFetchRealAtIndex(p_list, i, r_reals[i]))
			return false;
	
	return true;
}

bool MCProperListCreateWithArrayOfReal(const real64_t *p_reals, uindex_t p_size, MCProperListRef &r_list)
{
	bool t_success;
	t_success = true;
	
	MCAutoNumberRefArray t_numbers;
	
	if (t_success)
		t_success = t_numbers.New(p_size);
	
	for (uindex_t i = 0; t_success && i < p_size; i++)
		t_success = MCNumberCreateWithReal(p_reals[i], t_numbers[i]);
	
	if (t_success)
		t_success = MCProperListCreate((MCValueRef*)*t_numbers, p_size, r_list);
	
	return t_success;
}

bool MCProperListFetchAsArrayOfInteger(MCProperListRef p_list, uindex_t p_size, integer_t *r_integers)
{
	if (p_size != MCProperListGetLength(p_list))
		return false;
	
	for (uindex_t i = 0; i < p_size; i++)
		if (!MCProperListFetchIntegerAtIndex(p_list, i, r_integers[i]))
			return false;
	
	return true;
}

////////////////////////////////////////////////////////////////////////////////

inline MCGFloat MCGAffineTransformGetEffectiveScale(const MCGAffineTransform &p_transform)
{
	return MCMax(MCAbs(p_transform.a) + MCAbs(p_transform.c), MCAbs(p_transform.d) + MCAbs(p_transform.b));
}

////////////////////////////////////////////////////////////////////////////////

bool MCGPointParse(MCStringRef p_string, MCGPoint &r_point)
{
	bool t_success;
	t_success = nil;
	
	MCProperListRef t_items;
	t_items = nil;
	
	if (t_success)
		t_success = MCStringSplitByDelimiter(p_string, kMCCommaString, kMCStringOptionCompareExact, t_items);
	
	if (t_success)
		t_success = MCProperListGetLength(t_items) == 2;
	
	MCNumberRef t_x_num, t_y_num;
	t_x_num = t_y_num = nil;
	
	if (t_success)
		t_success = MCNumberParse((MCStringRef)MCProperListFetchElementAtIndex(t_items, 0), t_x_num);
	
	if (t_success)
		t_success = MCNumberParse((MCStringRef)MCProperListFetchElementAtIndex(t_items, 1), t_y_num);
	
	if (t_success)
		r_point = MCGPointMake(MCNumberFetchAsReal(t_x_num), MCNumberFetchAsReal(t_y_num));
	
	MCValueRelease(t_x_num);
	MCValueRelease(t_y_num);
	MCValueRelease(t_items);
	
	return t_success;
}

////////////////////////////////////////////////////////////////////////////////

bool MCSolveQuadraticEqn(MCGFloat p_a, MCGFloat p_b, MCGFloat p_c, MCGFloat &r_x_1, MCGFloat &r_x_2)
{
	MCGFloat t_det;
	t_det = p_b * p_b - 4 * p_a * p_c;
	
	if (t_det < 0)
		return false;
	
	MCGFloat t_sqrt;
	t_sqrt = sqrtf(t_det);
	
	r_x_1 = (-p_b + t_sqrt) / (2 * p_a);
	r_x_2 = (-p_b - t_sqrt) / (2 * p_a);
	
	return true;
}

////////////////////////////////////////////////////////////////////////////////

// Custom Types

void MCCanvasTypesInitialize();
void MCCanvasTypesFinalize();

MCTypeInfoRef kMCCanvasRectangleTypeInfo;
MCTypeInfoRef kMCCanvasPointTypeInfo;
MCTypeInfoRef kMCCanvasColorTypeInfo;
MCTypeInfoRef kMCCanvasTransformTypeInfo;
MCTypeInfoRef kMCCanvasImageTypeInfo;
MCTypeInfoRef kMCCanvasPaintTypeInfo;
MCTypeInfoRef kMCCanvasSolidPaintTypeInfo;
MCTypeInfoRef kMCCanvasPatternTypeInfo;
MCTypeInfoRef kMCCanvasGradientTypeInfo;
MCTypeInfoRef kMCCanvasGradientStopTypeInfo;
MCTypeInfoRef kMCCanvasPathTypeInfo;
MCTypeInfoRef kMCCanvasEffectTypeInfo;
MCTypeInfoRef kMCCanvasTypeInfo;

////////////////////////////////////////////////////////////////////////////////

// Error types

void MCCanvasErrorsInitialize();
void MCCanvasErrorsFinalize();

MCTypeInfoRef kMCCanvasRectangleListFormatErrorTypeInfo;
MCTypeInfoRef kMCCanvasPointListFormatErrorTypeInfo;
MCTypeInfoRef kMCCanvasColorListFormatErrorTypeInfo;
MCTypeInfoRef kMCCanvasScaleListFormatErrorTypeInfo;
MCTypeInfoRef kMCCanvasTranslationListFormatErrorTypeInfo;
MCTypeInfoRef kMCCanvasSkewListFormatErrorTypeInfo;
MCTypeInfoRef kMCCanvasRadiiListFormatErrorTypeInfo;
MCTypeInfoRef kMCCanvasImageSizeListFormatErrorTypeInfo;


////////////////////////////////////////////////////////////////////////////////

// Constant refs

MCCanvasTransformRef kMCCanvasIdentityTransform = nil;
MCCanvasColorRef kMCCanvasColorBlack = nil;

void MCCanvasConstantsInitialize();
void MCCanvasConstantsFinalize();

////////////////////////////////////////////////////////////////////////////////

bool MCCanvasBlendModeFromString(MCStringRef p_string, MCGBlendMode &r_blend_mode);
bool MCCanvasBlendModeToString(MCGBlendMode p_blend_mode, MCStringRef &r_string);
bool MCCanvasEffectTypeToString(MCCanvasEffectType p_type, MCStringRef &r_string);
bool MCCanvasGradientTypeFromString(MCStringRef p_string, MCGGradientFunction &r_type);
bool MCCanvasGradientTypeToString(MCGGradientFunction p_type, MCStringRef &r_string);
bool MCCanvasFillRuleFromString(MCStringRef p_string, MCGFillRule &r_fill_rule);
bool MCCanvasFillRuleToString(MCGFillRule p_fill_rule, MCStringRef &r_string);
bool MCCanvasImageFilterFromString(MCStringRef p_string, MCGImageFilter &r_filter);
bool MCCanvasImageFilterToString(MCGImageFilter p_filter, MCStringRef &r_string);
bool MCCanvasPathCommandToString(MCGPathCommand p_command, MCStringRef &r_string);
bool MCCanvasPathCommandFromString(MCStringRef p_string, MCGPathCommand &r_command);

////////////////////////////////////////////////////////////////////////////////

static MCNameRef s_blend_mode_map[kMCGBlendModeCount];
static MCNameRef s_transform_matrix_keys[9];
static MCNameRef s_effect_type_map[_MCCanvasEffectTypeCount];
static MCNameRef s_effect_property_map[_MCCanvasEffectPropertyCount];
static MCNameRef s_gradient_type_map[kMCGGradientFunctionCount];
static MCNameRef s_canvas_fillrule_map[kMCGFillRuleCount];
static MCNameRef s_image_filter_map[kMCGImageFilterCount];
static MCNameRef s_path_command_map[kMCGPathCommandCount];

////////////////////////////////////////////////////////////////////////////////

void MCCanvasStringsInitialize();
void MCCanvasStringsFinalize();

bool MCCanvasModuleInitialize()
{
	MCCanvasErrorsInitialize();
	MCCanvasStringsInitialize();
	MCCanvasTypesInitialize();
	MCCanvasConstantsInitialize();
    return true;
}

void MCCanvasModuleFinalize()
{
	MCCanvasErrorsFinalize();
	MCCanvasStringsFinalize();
	MCCanvasTypesFinalize();
	MCCanvasConstantsFinalize();
}

////////////////////////////////////////////////////////////////////////////////

MCGColor MCCanvasColorToMCGColor(MCCanvasColorRef p_color)
{
	__MCCanvasColorImpl *t_color;
	t_color = MCCanvasColorGet(p_color);
	return MCGColorMakeRGBA(t_color->red, t_color->green, t_color->blue, t_color->alpha);
}

////////////////////////////////////////////////////////////////////////////////

// Rectangle

static void __MCCanvasRectangleDestroy(MCValueRef p_value)
{
	// no-op
}

static bool __MCCanvasRectangleCopy(MCValueRef p_value, bool p_release, MCValueRef &r_copy)
{
	if (p_release)
		r_copy = p_value;
	else
		r_copy = MCValueRetain(p_value);
	return true;
}

static bool __MCCanvasRectangleEqual(MCValueRef p_left, MCValueRef p_right)
{
	if (p_left == p_right)
		return true;
	
	return MCMemoryCompare(MCValueGetExtraBytesPtr(p_left), MCValueGetExtraBytesPtr(p_right), sizeof(__MCCanvasRectangleImpl)) == 0;
}

static hash_t __MCCanvasRectangleHash(MCValueRef p_value)
{
	return MCHashBytes(MCValueGetExtraBytesPtr(p_value), sizeof(__MCCanvasRectangleImpl));
}

static bool __MCCanvasRectangleDescribe(MCValueRef p_value, MCStringRef &r_desc)
{
	// TODO - implement describe rectangle
	return false;
}

bool MCCanvasRectangleCreateWithMCGRectangle(const MCGRectangle &p_rect, MCCanvasRectangleRef &r_rectangle)
{
	bool t_success;
	t_success = true;
	
	MCCanvasRectangleRef t_rectangle;
	t_rectangle = nil;
	
	if (t_success)
		t_success = MCValueCreateCustom(kMCCanvasRectangleTypeInfo, sizeof(__MCCanvasRectangleImpl), t_rectangle);
	
	if (t_success)
	{
		*(MCCanvasRectangleGet(t_rectangle)) = p_rect;
		t_success = MCValueInter(t_rectangle, r_rectangle);
	}
	
	MCValueRelease(t_rectangle);
	
	return t_success;
}

__MCCanvasRectangleImpl *MCCanvasRectangleGet(MCCanvasRectangleRef p_rect)
{
	return (__MCCanvasRectangleImpl*)MCValueGetExtraBytesPtr(p_rect);
}

void MCCanvasRectangleGetMCGRectangle(MCCanvasRectangleRef p_rect, MCGRectangle &r_rect)
{
	r_rect = *MCCanvasRectangleGet(p_rect);
}

//////////

bool MCProperListToRectangle(MCProperListRef p_list, MCGRectangle &r_rectangle)
{
	bool t_success;
	t_success = true;
	
	real64_t t_rect[4];
	
	if (t_success)
		t_success = MCProperListFetchAsArrayOfReal(p_list, 4, t_rect);
		
	if (t_success)
		r_rectangle = MCGRectangleMake(t_rect[0], t_rect[1], t_rect[2] - t_rect[0], t_rect[3] - t_rect[1]);
	else
		MCCanvasThrowError(kMCCanvasRectangleListFormatErrorTypeInfo);
	
	return t_success;
}

// Constructors

void MCCanvasRectangleMakeWithLTRB(MCCanvasFloat p_left, MCCanvasFloat p_top, MCCanvasFloat p_right, MCCanvasFloat p_bottom, MCCanvasRectangleRef &r_rect)
{
	/* UNCHECKED */ MCCanvasRectangleCreateWithMCGRectangle(MCGRectangleMake(p_left, p_top, p_right - p_left, p_bottom - p_top), r_rect);
}

void MCCanvasRectangleMakeWithList(MCProperListRef p_list, MCCanvasRectangleRef &r_rect)
{
	MCGRectangle t_rect;
	if (!MCProperListToRectangle(p_list, t_rect))
		return;
	
	/* UNCHECKED */ MCCanvasRectangleCreateWithMCGRectangle(t_rect, r_rect);
}

// Properties

void MCCanvasRectangleSetMCGRectangle(const MCGRectangle &p_rect, MCCanvasRectangleRef &x_rect)
{
	MCCanvasRectangleRef t_rect;
	if (!MCCanvasRectangleCreateWithMCGRectangle(p_rect, t_rect))
		return;
	
	MCValueAssign(x_rect, t_rect);
	MCValueRelease(t_rect);
}

void MCCanvasRectangleGetLeft(MCCanvasRectangleRef p_rect, MCCanvasFloat &r_left)
{
	MCGRectangle t_rect;
	MCCanvasRectangleGetMCGRectangle(p_rect, t_rect);
	r_left = t_rect.origin.x;
}

void MCCanvasRectangleSetLeft(MCCanvasFloat p_left, MCCanvasRectangleRef &x_rect)
{
	MCGRectangle t_rect;
	MCCanvasRectangleGetMCGRectangle(x_rect, t_rect);
	t_rect.origin.x = p_left;

	MCCanvasRectangleSetMCGRectangle(t_rect, x_rect);
}

void MCCanvasRectangleGetTop(MCCanvasRectangleRef p_rect, MCCanvasFloat &r_top)
{
	MCGRectangle t_rect;
	MCCanvasRectangleGetMCGRectangle(p_rect, t_rect);
	r_top = t_rect.origin.y;
}

void MCCanvasRectangleSetTop(MCCanvasFloat p_top, MCCanvasRectangleRef &x_rect)
{
	MCGRectangle t_rect;
	MCCanvasRectangleGetMCGRectangle(x_rect, t_rect);
	t_rect.origin.y = p_top;
	
	MCCanvasRectangleSetMCGRectangle(t_rect, x_rect);
}

void MCCanvasRectangleGetRight(MCCanvasRectangleRef p_rect, MCCanvasFloat &r_right)
{
	MCGRectangle t_rect;
	MCCanvasRectangleGetMCGRectangle(p_rect, t_rect);
	r_right = t_rect.origin.x + t_rect.size.width;
}

void MCCanvasRectangleSetRight(MCCanvasFloat p_right, MCCanvasRectangleRef &x_rect)
{
	MCGRectangle t_rect;
	MCCanvasRectangleGetMCGRectangle(x_rect, t_rect);
	t_rect.origin.x = p_right - t_rect.size.width;
	
	MCCanvasRectangleSetMCGRectangle(t_rect, x_rect);
}

void MCCanvasRectangleGetBottom(MCCanvasRectangleRef p_rect, MCCanvasFloat &r_bottom)
{
	MCGRectangle t_rect;
	MCCanvasRectangleGetMCGRectangle(p_rect, t_rect);
	r_bottom = t_rect.origin.y + t_rect.size.height;
}

void MCCanvasRectangleSetBottom(MCCanvasFloat p_bottom, MCCanvasRectangleRef &x_rect)
{
	MCGRectangle t_rect;
	MCCanvasRectangleGetMCGRectangle(x_rect, t_rect);
	t_rect.origin.y = p_bottom - t_rect.size.height;
	
	MCCanvasRectangleSetMCGRectangle(t_rect, x_rect);
}

void MCCanvasRectangleGetWidth(MCCanvasRectangleRef p_rect, MCCanvasFloat &r_width)
{
	MCGRectangle t_rect;
	MCCanvasRectangleGetMCGRectangle(p_rect, t_rect);
	r_width = t_rect.size.width;
}

void MCCanvasRectangleSetWidth(MCCanvasFloat p_width, MCCanvasRectangleRef &x_rect)
{
	MCGRectangle t_rect;
	MCCanvasRectangleGetMCGRectangle(x_rect, t_rect);
	t_rect.size.width = p_width;
	
	MCCanvasRectangleSetMCGRectangle(t_rect, x_rect);
}

void MCCanvasRectangleGetHeight(MCCanvasRectangleRef p_rect, MCCanvasFloat &r_height)
{
	MCGRectangle t_rect;
	MCCanvasRectangleGetMCGRectangle(p_rect, t_rect);
	r_height = t_rect.size.height;
}

void MCCanvasRectangleSetHeight(MCCanvasFloat p_height, MCCanvasRectangleRef &x_rect)
{
	MCGRectangle t_rect;
	MCCanvasRectangleGetMCGRectangle(x_rect, t_rect);
	t_rect.size.height = p_height;
	
	MCCanvasRectangleSetMCGRectangle(t_rect, x_rect);
}

////////////////////////////////////////////////////////////////////////////////

// Point

static void __MCCanvasPointDestroy(MCValueRef p_value)
{
	// no-op
}

static bool __MCCanvasPointCopy(MCValueRef p_value, bool p_release, MCValueRef &r_copy)
{
	if (p_release)
		r_copy = p_value;
	else
		r_copy = MCValueRetain(p_value);
	return true;
}

static bool __MCCanvasPointEqual(MCValueRef p_left, MCValueRef p_right)
{
	if (p_left == p_right)
		return true;
	
	return MCMemoryCompare(MCValueGetExtraBytesPtr(p_left), MCValueGetExtraBytesPtr(p_left), sizeof(__MCCanvasPointImpl)) == 0;
}

static hash_t __MCCanvasPointHash(MCValueRef p_value)
{
	return MCHashBytes(MCValueGetExtraBytesPtr(p_value), sizeof(__MCCanvasPointImpl));
}

static bool __MCCanvasPointDescribe(MCValueRef p_value, MCStringRef &r_desc)
{
	// TODO - implement describe point
	return false;
}

bool MCCanvasPointCreateWithMCGPoint(const MCGPoint &p_point, MCCanvasPointRef &r_point)
{
	bool t_success;
	t_success = true;
	
	MCCanvasPointRef t_point;
	t_point = nil;
	
	if (t_success)
		t_success = MCValueCreateCustom(kMCCanvasPointTypeInfo, sizeof(__MCCanvasPointImpl), t_point);
	
	if (t_success)
	{
		*MCCanvasPointGet(t_point) = p_point;
		t_success = MCValueInter(t_point, r_point);
	}
	
	MCValueRelease(t_point);
	
	return t_success;
}

__MCCanvasPointImpl *MCCanvasPointGet(MCCanvasPointRef p_point)
{
	return (__MCCanvasPointImpl*)MCValueGetExtraBytesPtr(p_point);
}

void MCCanvasPointGetMCGPoint(MCCanvasPointRef p_point, MCGPoint &r_point)
{
	r_point = *MCCanvasPointGet(p_point);
}

//////////

bool MCProperListToPoint(MCProperListRef p_list, MCGPoint &r_point)
{
	real64_t t_point[2];
	if (!MCProperListFetchAsArrayOfReal(p_list, 2, t_point))
	{
		MCCanvasThrowError(kMCCanvasPointListFormatErrorTypeInfo);
		return false;
	}

	r_point = MCGPointMake(t_point[0], t_point[1]);
	
	return true;
}

bool MCProperListFromPoint(const MCGPoint &p_point, MCProperListRef &r_list)
{
	real64_t t_point[2];
	t_point[0] = p_point.x;
	t_point[1] = p_point.y;
	
	return MCProperListCreateWithArrayOfReal(t_point, 2, r_list);
}

// Constructors

void MCCanvasPointMake(MCCanvasFloat p_x, MCCanvasFloat p_y, MCCanvasPointRef &r_point)
{
	/* UNCHECKED */ MCCanvasPointCreateWithMCGPoint(MCGPointMake(p_x, p_y), r_point);
}

void MCCanvasPointMakeWithList(MCProperListRef p_list, MCCanvasPointRef &r_point)
{
	MCGPoint t_point;
	if (!MCProperListToPoint(p_list, t_point))
		return;
	
	/* UNCHECKED */ MCCanvasPointCreateWithMCGPoint(t_point, r_point);
}

// Properties

void MCCanvasPointSetMCGPoint(const MCGPoint &p_point, MCCanvasPointRef &x_point)
{
	MCCanvasPointRef t_point;
	if (!MCCanvasPointCreateWithMCGPoint(p_point, t_point))
		return;
	MCValueAssign(x_point, t_point);
	MCValueRelease(t_point);
}

void MCCanvasPointGetX(MCCanvasPointRef p_point, MCCanvasFloat &r_x)
{
	MCGPoint t_point;
	MCCanvasPointGetMCGPoint(p_point, t_point);
	r_x = t_point.x;
}

void MCCanvasPointSetX(MCCanvasFloat p_x, MCCanvasPointRef &x_point)
{
	MCGPoint t_point;
	MCCanvasPointGetMCGPoint(x_point, t_point);
	t_point.x = p_x;
	
	MCCanvasPointSetMCGPoint(t_point, x_point);
}

void MCCanvasPointGetY(MCCanvasPointRef p_point, MCCanvasFloat &r_y)
{
	MCGPoint t_point;
	MCCanvasPointGetMCGPoint(p_point, t_point);
	r_y = t_point.y;
}

void MCCanvasPointSetY(MCCanvasFloat p_y, MCCanvasPointRef &x_point)
{
	MCGPoint t_point;
	MCCanvasPointGetMCGPoint(x_point, t_point);
	t_point.y = p_y;
	
	MCCanvasPointSetMCGPoint(t_point, x_point);
}

////////////////////////////////////////////////////////////////////////////////

// Color

// MCCanvasColorRef Type methods

static void __MCCanvasColorDestroy(MCValueRef p_value)
{
	// no-op
}

static bool __MCCanvasColorCopy(MCValueRef p_value, bool p_release, MCValueRef &r_copy)
{
	if (p_release)
		r_copy = p_value;
	else
		r_copy = MCValueRetain(p_value);
	return true;
}

static bool __MCCanvasColorEqual(MCValueRef p_left, MCValueRef p_right)
{
	if (p_left == p_right)
		return true;
	
	return MCMemoryCompare(MCValueGetExtraBytesPtr(p_left), MCValueGetExtraBytesPtr(p_right), sizeof(__MCCanvasColorImpl)) == 0;
}

static hash_t __MCCanvasColorHash(MCValueRef p_value)
{
	return MCHashBytes(MCValueGetExtraBytesPtr(p_value), sizeof(__MCCanvasColorImpl));
}

static bool __MCCanvasColorDescribe(MCValueRef p_value, MCStringRef &r_desc)
{
	// TODO - implement describe
	return false;
}

//////////

bool MCCanvasColorCreate(const __MCCanvasColorImpl &p_color, MCCanvasColorRef &r_color)
{
	bool t_success;
	t_success = true;
	
	MCCanvasColorRef t_color;
	t_color = nil;
	
	t_success = MCValueCreateCustom(kMCCanvasColorTypeInfo, sizeof(__MCCanvasColorImpl), t_color);
	
	if (t_success)
	{
		*MCCanvasColorGet(t_color) = p_color;
		t_success = MCValueInterAndRelease(t_color, r_color);
        if (!t_success)
            MCValueRelease(t_color);
	}
	return t_success;
}

__MCCanvasColorImpl *MCCanvasColorGet(MCCanvasColorRef p_color)
{
	return (__MCCanvasColorImpl*)MCValueGetExtraBytesPtr(p_color);
}

static inline __MCCanvasColorImpl MCCanvasColorImplMake(MCCanvasFloat p_red, MCCanvasFloat p_green, MCCanvasFloat p_blue, MCCanvasFloat p_alpha)
{
	__MCCanvasColorImpl t_color;
	t_color.red = p_red;
	t_color.green = p_green;
	t_color.blue = p_blue;
	t_color.alpha = p_alpha;
	
	return t_color;
}

bool MCCanvasColorCreateWithRGBA(MCCanvasFloat p_red, MCCanvasFloat p_green, MCCanvasFloat p_blue, MCCanvasFloat p_alpha, MCCanvasColorRef &r_color)
{
	return MCCanvasColorCreate(MCCanvasColorImplMake(p_red, p_green, p_blue, p_alpha), r_color);
}

MCCanvasFloat MCCanvasColorGetRed(MCCanvasColorRef color)
{
	return MCCanvasColorGet(color)->red;
}

MCCanvasFloat MCCanvasColorGetGreen(MCCanvasColorRef color)
{
	return MCCanvasColorGet(color)->green;
}

MCCanvasFloat MCCanvasColorGetBlue(MCCanvasColorRef color)
{
	return MCCanvasColorGet(color)->blue;
}

MCCanvasFloat MCCanvasColorGetAlpha(MCCanvasColorRef color)
{
	return MCCanvasColorGet(color)->alpha;
}

bool MCProperListToRGBA(MCProperListRef p_list, MCCanvasFloat &r_red, MCCanvasFloat &r_green, MCCanvasFloat &r_blue, MCCanvasFloat &r_alpha)
{
	bool t_success;
	t_success = true;
	
	uindex_t t_length;
	t_length = MCProperListGetLength(p_list);
	
	real64_t t_rgba[4];
	
	if (t_success)
		t_success = t_length == 3 || t_length == 4;
	
	if (t_success)
		t_success = MCProperListFetchAsArrayOfReal(p_list, t_length, t_rgba);
	
	if (t_success)
	{
		if (t_length == 3)
			t_rgba[3] = 1.0; // set default alpha value of 1.0
	}
	
	if (t_success)
	{
		r_red = t_rgba[0];
		r_green = t_rgba[1];
		r_blue = t_rgba[2];
		r_alpha = t_rgba[3];
	}
	else
		MCCanvasThrowError(kMCCanvasColorListFormatErrorTypeInfo);
	
	return t_success;
}

// Constructors

void MCCanvasColorMakeRGBA(MCCanvasFloat p_red, MCCanvasFloat p_green, MCCanvasFloat p_blue, MCCanvasFloat p_alpha, MCCanvasColorRef &r_color)
{
	/* UNCHECKED */ MCCanvasColorCreate(MCCanvasColorImplMake(p_red, p_blue, p_green, p_alpha), r_color);
}

void MCCanvasColorMakeWithList(MCProperListRef p_color, MCCanvasColorRef &r_color)
{
	MCCanvasFloat t_red, t_green, t_blue, t_alpha;
	if (!MCProperListToRGBA(p_color, t_red, t_green, t_blue, t_alpha))
		return;
	
	/* UNCHECKED */ MCCanvasColorCreateWithRGBA(t_red, t_green, t_blue, t_alpha, r_color);
}

//////////

// Properties

void MCCanvasColorSet(const __MCCanvasColorImpl &p_color, MCCanvasColorRef &x_color)
{
	MCCanvasColorRef t_color;
	if (!MCCanvasColorCreate(p_color, t_color))
		return;
	MCValueAssign(x_color, t_color);
	MCValueRelease(t_color);
}

void MCCanvasColorGetRed(MCCanvasColorRef p_color, MCCanvasFloat &r_red)
{
	r_red = MCCanvasColorGetRed(p_color);
}

void MCCanvasColorSetRed(MCCanvasFloat p_red, MCCanvasColorRef &x_color)
{
	__MCCanvasColorImpl *t_color;
	t_color = MCCanvasColorGet(x_color);
	
	if (t_color->red == p_red)
		return;
	
	t_color->red = p_red;
	MCCanvasColorSet(*t_color, x_color);
}

void MCCanvasColorGetGreen(MCCanvasColorRef p_color, MCCanvasFloat &r_green)
{
	r_green = MCCanvasColorGetGreen(p_color);
}

void MCCanvasColorSetGreen(MCCanvasFloat p_green, MCCanvasColorRef &x_color)
{
	__MCCanvasColorImpl *t_color;
	t_color = MCCanvasColorGet(x_color);
	
	if (t_color->green == p_green)
		return;
	
	t_color->green = p_green;
	MCCanvasColorSet(*t_color, x_color);
}

void MCCanvasColorGetBlue(MCCanvasColorRef p_color, MCCanvasFloat &r_blue)
{
	r_blue = MCCanvasColorGetBlue(p_color);
}

void MCCanvasColorSetBlue(MCCanvasFloat p_blue, MCCanvasColorRef &x_color)
{
	__MCCanvasColorImpl *t_color;
	t_color = MCCanvasColorGet(x_color);
	
	if (t_color->blue == p_blue)
		return;
	
	t_color->blue = p_blue;
	MCCanvasColorSet(*t_color, x_color);
}

void MCCanvasColorGetAlpha(MCCanvasColorRef p_color, MCCanvasFloat &r_alpha)
{
	r_alpha = MCCanvasColorGetAlpha(p_color);
}

void MCCanvasColorSetAlpha(MCCanvasFloat p_alpha, MCCanvasColorRef &x_color)
{
	__MCCanvasColorImpl *t_color;
	t_color = MCCanvasColorGet(x_color);
	
	if (t_color->alpha == p_alpha)
		return;
	
	t_color->alpha = p_alpha;
	MCCanvasColorSet(*t_color, x_color);
}

////////////////////////////////////////////////////////////////////////////////

// Transform

// MCCanvasColorRef Type methods

static void __MCCanvasTransformDestroy(MCValueRef p_value)
{
	// no-op
}

static bool __MCCanvasTransformCopy(MCValueRef p_value, bool p_release, MCValueRef &r_copy)
{
	if (p_release)
		r_copy = p_value;
	else
		r_copy = MCValueRetain(p_value);
	return true;
}

static bool __MCCanvasTransformEqual(MCValueRef p_left, MCValueRef p_right)
{
	if (p_left == p_right)
		return true;
	
	return MCMemoryCompare(MCValueGetExtraBytesPtr(p_left), MCValueGetExtraBytesPtr(p_right), sizeof(__MCCanvasTransformImpl)) == 0;
}

static hash_t __MCCanvasTransformHash(MCValueRef p_value)
{
	return MCHashBytes(MCValueGetExtraBytesPtr(p_value), sizeof(__MCCanvasTransformImpl));
}

static bool __MCCanvasTransformDescribe(MCValueRef p_value, MCStringRef &r_desc)
{
	// TODO - implement describe
	return false;
}

//////////

bool MCCanvasTransformCreateWithMCGAffineTransform(const MCGAffineTransform &p_transform, MCCanvasTransformRef &r_transform)
{
	bool t_success;
	t_success = true;
	
	MCCanvasTransformRef t_transform;
	t_transform = nil;
	
	t_success = MCValueCreateCustom(kMCCanvasTransformTypeInfo, sizeof(__MCCanvasTransformImpl), t_transform);
	
	if (t_success)
	{
		*MCCanvasTransformGet(t_transform) = p_transform;
		t_success = MCValueInter(t_transform, r_transform);
	}
	
	MCValueRelease(t_transform);
	
	return t_success;
}

__MCCanvasTransformImpl *MCCanvasTransformGet(MCCanvasTransformRef p_transform)
{
	return (__MCCanvasTransformImpl*)MCValueGetExtraBytesPtr(p_transform);
}

//////////

// special case for scale parameters, which may have one or two values
bool MCProperListToScale(MCProperListRef p_list, MCGPoint &r_scale)
{
	bool t_success;
	t_success = true;
	
	uindex_t t_length;
	t_length = MCProperListGetLength(p_list);
	
	real64_t t_scale[2];
	
	if (t_success)
		t_success = t_length == 1 || t_length == 2;
	
	if (t_success)
		t_success = MCProperListFetchAsArrayOfReal(p_list, t_length, t_scale);
	
	if (t_success)
	{
		if (t_length == 1)
			t_scale[1] = t_scale[0];
		r_scale = MCGPointMake(t_scale[0], t_scale[1]);
	}
	else
		MCCanvasThrowError(kMCCanvasScaleListFormatErrorTypeInfo);
	
	return t_success;
}

bool MCProperListToSkew(MCProperListRef p_list, MCGPoint &r_skew)
{
	bool t_success;
	t_success = true;
	
	real64_t t_skew[2];
	
	t_success = MCProperListFetchAsArrayOfReal(p_list, 2, t_skew);
	
	if (t_success)
		r_skew = MCGPointMake(t_skew[0], t_skew[1]);
	else
		MCCanvasThrowError(kMCCanvasSkewListFormatErrorTypeInfo);
	
	return t_success;
}

bool MCProperListToTranslation(MCProperListRef p_list, MCGPoint &r_translation)
{
	bool t_success;
	t_success = true;
	
	real64_t t_translation[2];
	
	t_success = MCProperListFetchAsArrayOfReal(p_list, 2, t_translation);
	
	if (t_success)
		r_translation = MCGPointMake(t_translation[0], t_translation[1]);
	else
		MCCanvasThrowError(kMCCanvasTranslationListFormatErrorTypeInfo);
	
	return t_success;
}

// Constructors

void MCCanvasTransformMake(const MCGAffineTransform &p_transform, MCCanvasTransformRef &r_transform)
{
	/* UNCHECKED */ MCCanvasTransformCreateWithMCGAffineTransform(p_transform, r_transform);
}

void MCCanvasTransformMakeIdentity(MCCanvasTransformRef &r_transform)
{
	r_transform = MCValueRetain(kMCCanvasIdentityTransform);
}

void MCCanvasTransformMakeScale(MCCanvasFloat p_xscale, MCCanvasFloat p_yscale, MCCanvasTransformRef &r_transform)
{
	MCCanvasTransformMake(MCGAffineTransformMakeScale(p_xscale, p_yscale), r_transform);
}

void MCCanvasTransformMakeScaleWithList(MCProperListRef p_scale, MCCanvasTransformRef &r_transform)
{
	MCGPoint t_scale;
	if (!MCProperListToScale(p_scale, t_scale))
		return;
	
	MCCanvasTransformMakeScale(t_scale.x, t_scale.y, r_transform);
}

void MCCanvasTransformMakeRotation(MCCanvasFloat p_angle, MCCanvasTransformRef &r_transform)
{
	MCCanvasTransformMake(MCGAffineTransformMakeRotation(MCCanvasAngleToDegrees(p_angle)), r_transform);
}

void MCCanvasTransformMakeTranslation(MCCanvasFloat p_x, MCCanvasFloat p_y, MCCanvasTransformRef &r_transform)
{
	MCCanvasTransformMake(MCGAffineTransformMakeTranslation(p_x, p_y), r_transform);
}

void MCCanvasTransformMakeTranslationWithList(MCProperListRef p_translation, MCCanvasTransformRef &r_transform)
{
	MCGPoint t_translation;
	if (!MCProperListToTranslation(p_translation, t_translation))
		return;
	
	MCCanvasTransformMakeTranslation(t_translation.x, t_translation.y, r_transform);
}

void MCCanvasTransformMakeSkew(MCCanvasFloat p_x, MCCanvasFloat p_y, MCCanvasTransformRef &r_transform)
{
	MCCanvasTransformMake(MCGAffineTransformMakeSkew(p_x, p_y), r_transform);
}

void MCCanvasTransformMakeSkewWithList(MCProperListRef p_skew, MCCanvasTransformRef &r_transform)
{
	MCGPoint t_skew;
	if (!MCProperListToSkew(p_skew, t_skew))
		return;
	
	MCCanvasTransformMakeSkew(t_skew.x, t_skew.y, r_transform);
}

void MCCanvasTransformMakeWithMatrixValues(MCCanvasFloat p_a, MCCanvasFloat p_b, MCCanvasFloat p_c, MCCanvasFloat p_d, MCCanvasFloat p_tx, MCCanvasFloat p_ty, MCCanvasTransformRef &r_transform)
{
	MCCanvasTransformMake(MCGAffineTransformMake(p_a, p_b, p_c, p_d, p_tx, p_ty), r_transform);
}

//////////

// Properties

void MCCanvasTransformSetMCGAffineTransform(const MCGAffineTransform &p_transform, MCCanvasTransformRef &x_transform)
{
	MCCanvasTransformRef t_transform;
	if (!MCCanvasTransformCreateWithMCGAffineTransform(p_transform, t_transform))
		return;
	MCValueAssign(x_transform, t_transform);
	MCValueRelease(t_transform);
}

void MCCanvasTransformGetMatrix(MCCanvasTransformRef p_transform, MCArrayRef &r_matrix)
{
	bool t_success;
	t_success = true;
	
	MCGAffineTransform *t_transform;
	t_transform = MCCanvasTransformGet(p_transform);
	
	MCArrayRef t_matrix;
	t_matrix = nil;
	
	if (t_success)
		t_success = MCArrayCreateMutable(t_matrix);
	
	if (t_success)
		t_success = MCArrayStoreReal(t_matrix, s_transform_matrix_keys[0], t_transform->a) &&
		MCArrayStoreReal(t_matrix, s_transform_matrix_keys[1], t_transform->b) &&
		MCArrayStoreReal(t_matrix, s_transform_matrix_keys[2], 0) &&
		MCArrayStoreReal(t_matrix, s_transform_matrix_keys[3], t_transform->c) &&
		MCArrayStoreReal(t_matrix, s_transform_matrix_keys[4], t_transform->d) &&
		MCArrayStoreReal(t_matrix, s_transform_matrix_keys[5], 0) &&
		MCArrayStoreReal(t_matrix, s_transform_matrix_keys[6], t_transform->tx) &&
		MCArrayStoreReal(t_matrix, s_transform_matrix_keys[7], t_transform->ty) &&
		MCArrayStoreReal(t_matrix, s_transform_matrix_keys[8], 1);
		
	if (t_success)
		t_success = MCArrayCopy(t_matrix, r_matrix);
	MCValueRelease(t_matrix);
}

void MCCanvasTransformSetMatrix(MCArrayRef p_matrix, MCCanvasTransformRef &x_transform)
{
	bool t_success;
	t_success = true;
	
	real64_t a, b, c, d, tx, ty;
	a = b = c = d = tx = ty = 0.0;
	t_success =
		MCArrayFetchReal(p_matrix, s_transform_matrix_keys[0], a) &&
		MCArrayFetchReal(p_matrix, s_transform_matrix_keys[1], b) &&
		MCArrayFetchReal(p_matrix, s_transform_matrix_keys[3], c) &&
		MCArrayFetchReal(p_matrix, s_transform_matrix_keys[4], d) &&
		MCArrayFetchReal(p_matrix, s_transform_matrix_keys[6], tx) &&
		MCArrayFetchReal(p_matrix, s_transform_matrix_keys[7], ty);

	if (!t_success)
	{
		// TODO - throw matrix array keys error
		return;
	}
	
	MCCanvasTransformSetMCGAffineTransform(MCGAffineTransformMake(a, b, c, d, tx, ty), x_transform);
}

void MCCanvasTransformGetInverse(MCCanvasTransformRef p_transform, MCCanvasTransformRef &r_transform)
{
	MCCanvasTransformMake(MCGAffineTransformInvert(*MCCanvasTransformGet(p_transform)), r_transform);
}

// T = Tscale * Trotate * Tskew * Ttranslate

// Ttranslate:
// / 1 0 tx \
// | 0 1 ty |
// \ 0 0  1 /

// Tscale * Trotate * Tskew:
// / a  c \   / ScaleX       0 \   /  Cos(r)  Sin(r) \   / 1  Skew \   /  ScaleX * Cos(r)   ScaleX * Skew * Cos(r) + ScaleX * Sin(r) \
// \ b  d / = \      0  ScaleY / * \ -Sin(r)  Cos(r) / * \ 0     1 / = \ -ScaleY * Sin(r)  -ScaleY * Skew * Sin(r) + ScaleY * Cos(r) /

bool MCCanvasTransformDecompose(const MCGAffineTransform &p_transform, MCGPoint &r_scale, MCCanvasFloat &r_rotation, MCGPoint &r_skew, MCGPoint &r_translation)
{
	MCGFloat t_r, t_skew;
	MCGPoint t_scale, t_trans;
	
	t_trans = MCGPointMake(p_transform.tx, p_transform.ty);
	
	// if a == 0, take r to be pi/2 radians
	if (p_transform.a == 0)
	{
		t_r = M_PI / 2;
		// b == -ScaleY, c == ScaleX
		t_scale = MCGPointMake(p_transform.c, -p_transform.b);
		// d = -ScaleY * Skew => Skew = d / -ScaleY => Skew = d / b
		if (p_transform.b == 0)
			return false;
		t_skew = p_transform.d / p_transform.b;
	}
	// if b == 0, take r to be 0 radians
	else if (p_transform.b == 0)
	{
		t_r = 0;
		// a == ScaleX, d == ScaleY
		t_scale = MCGPointMake(p_transform.a, p_transform.d);
		// c == ScaleX * Skew => Skew == c / ScaleX => Skew = c / a
		if (p_transform.a == 0)
			return false;
		t_skew = p_transform.c / p_transform.a;
	}
	else
	{
		// Skew^2 + (-c/a - d/b) * Skew + (c/a * d/b - 1) = 0
		MCGFloat t_c_div_a, t_d_div_b;
		t_c_div_a = p_transform.c / p_transform.a;
		t_d_div_b = p_transform.d / p_transform.b;
		
		MCGFloat t_x1, t_x2;
		if (!MCSolveQuadraticEqn(1, -t_c_div_a -t_d_div_b, t_c_div_a * t_d_div_b - 1, t_x1, t_x2))
			return false;
		
		// choose skew with smallest absolute value
		if (MCAbs(t_x1) < MCAbs(t_x2))
			t_skew = t_x1;
		else
			t_skew = t_x2;
		
		// Tan(r) = c/a -Skew
		t_r = atan(t_c_div_a - t_skew);
		
		t_scale = MCGPointMake(p_transform.a / cosf(t_r), -p_transform.b / sinf(t_r));
	}
	
	r_scale = t_scale;
	r_rotation = t_r;
	r_skew = MCGPointMake(t_skew, 0);
	r_translation = t_trans;
	
	return true;
}

MCGAffineTransform MCCanvasTransformCompose(const MCGPoint &p_scale, MCCanvasFloat p_rotation, const MCGPoint &p_skew, const MCGPoint &p_translation)
{
	MCGAffineTransform t_transform;
	t_transform = MCGAffineTransformMakeScale(p_scale.x, p_scale.y);
	t_transform = MCGAffineTransformConcat(t_transform, MCGAffineTransformMakeRotation(p_rotation));
	t_transform = MCGAffineTransformConcat(t_transform, MCGAffineTransformMakeSkew(p_skew.x, p_skew.y));
	t_transform = MCGAffineTransformConcat(t_transform, MCGAffineTransformMakeTranslation(p_translation.x, p_translation.y));
	
	return t_transform;
}

void MCCanvasTransformGetScaleAsList(MCCanvasTransformRef p_transform, MCProperListRef &r_scale)
{
	MCGPoint t_scale, t_skew, t_translation;
	MCCanvasFloat t_rotation;
	
	if (!MCCanvasTransformDecompose(*MCCanvasTransformGet(p_transform), t_scale, t_rotation, t_skew, t_translation))
	{
		// TODO - throw transform decompose error
		return;
	}
	
	/* UNCHECKED */ MCProperListFromPoint(t_scale, r_scale);
}

void MCCanvasTransformSetScaleAsList(MCProperListRef p_scale, MCCanvasTransformRef &x_transform)
{
	MCGPoint t_scale, t_skew, t_translation;
	MCCanvasFloat t_rotation;
	
	if (!MCCanvasTransformDecompose(*MCCanvasTransformGet(x_transform), t_scale, t_rotation, t_skew, t_translation))
	{
		// TODO - throw transform decompose error
		return;
	}
	
	if (!MCProperListToScale(p_scale, t_scale))
		return;
		
	MCCanvasTransformSetMCGAffineTransform(MCCanvasTransformCompose(t_scale, t_rotation, t_skew, t_translation), x_transform);
}

void MCCanvasTransformGetRotation(MCCanvasTransformRef p_transform, MCCanvasFloat &r_rotation)
{
	MCGPoint t_scale, t_skew, t_translation;
	MCCanvasFloat t_rotation;
	
	if (!MCCanvasTransformDecompose(*MCCanvasTransformGet(p_transform), t_scale, t_rotation, t_skew, t_translation))
	{
		// TODO - throw transform decompose error
		return;
	}
	r_rotation = MCCanvasAngleFromRadians(t_rotation);
}

void MCCanvasTransformSetRotation(MCCanvasFloat p_rotation, MCCanvasTransformRef &x_transform)
{
	MCGPoint t_scale, t_skew, t_translation;
	MCCanvasFloat t_rotation;
	
	if (!MCCanvasTransformDecompose(*MCCanvasTransformGet(x_transform), t_scale, t_rotation, t_skew, t_translation))
	{
		// TODO - throw transform decompose error
		return;
	}
	
	MCCanvasTransformSetMCGAffineTransform(MCCanvasTransformCompose(t_scale, MCCanvasAngleToRadians(p_rotation), t_skew, t_translation), x_transform);
}

void MCCanvasTransformGetSkewAsList(MCCanvasTransformRef p_transform, MCProperListRef &r_skew)
{
	MCGPoint t_scale, t_skew, t_translation;
	MCCanvasFloat t_rotation;
	
	if (!MCCanvasTransformDecompose(*MCCanvasTransformGet(p_transform), t_scale, t_rotation, t_skew, t_translation))
	{
		// TODO - throw transform decompose error
		return;
	}
	
	/* UNCHECKED */ MCProperListFromPoint(t_skew, r_skew);
}

void MCCanvasTransformSetSkewAsList(MCProperListRef p_skew, MCCanvasTransformRef &x_transform)
{
	MCGPoint t_scale, t_skew, t_translation;
	MCCanvasFloat t_rotation;
	
	if (!MCCanvasTransformDecompose(*MCCanvasTransformGet(x_transform), t_scale, t_rotation, t_skew, t_translation))
	{
		// TODO - throw transform decompose error
		return;
	}
	
	if (!MCProperListToSkew(p_skew, t_skew))
		return;
	
	MCCanvasTransformSetMCGAffineTransform(MCCanvasTransformCompose(t_scale, t_rotation, t_skew, t_translation), x_transform);
}

void MCCanvasTransformGetTranslationAsList(MCCanvasTransformRef p_transform, MCProperListRef &r_translation)
{
	MCGPoint t_scale, t_skew, t_translation;
	MCCanvasFloat t_rotation;
	
	if (!MCCanvasTransformDecompose(*MCCanvasTransformGet(p_transform), t_scale, t_rotation, t_skew, t_translation))
	{
		// TODO - throw transform decompose error
		return;
	}
	
	/* UNCHECKED */ MCProperListFromPoint(t_translation, r_translation);
}

void MCCanvasTransformSetTranslationAsList(MCProperListRef p_translation, MCCanvasTransformRef &x_transform)
{
	MCGPoint t_scale, t_skew, t_translation;
	MCCanvasFloat t_rotation;
	
	if (!MCCanvasTransformDecompose(*MCCanvasTransformGet(x_transform), t_scale, t_rotation, t_skew, t_translation))
	{
		// TODO - throw transform decompose error
		return;
	}
	
	if (!MCProperListToTranslation(p_translation, t_translation))
		return;
	
	MCCanvasTransformSetMCGAffineTransform(MCCanvasTransformCompose(t_scale, t_rotation, t_skew, t_translation), x_transform);
}

//////////

// Operations

void MCCanvasTransformConcat(MCCanvasTransformRef &x_transform, const MCGAffineTransform &p_transform)
{
	MCCanvasTransformSetMCGAffineTransform(MCGAffineTransformConcat(p_transform, *MCCanvasTransformGet(x_transform)), x_transform);
}

void MCCanvasTransformConcat(MCCanvasTransformRef &x_transform, MCCanvasTransformRef p_transform)
{
	MCCanvasTransformConcat(x_transform, *MCCanvasTransformGet(p_transform));
}

void MCCanvasTransformScale(MCCanvasTransformRef &x_transform, MCCanvasFloat p_x_scale, MCCanvasFloat p_y_scale)
{
	MCCanvasTransformConcat(x_transform, MCGAffineTransformMakeScale(p_x_scale, p_y_scale));
}

void MCCanvasTransformScaleWithList(MCCanvasTransformRef &x_transform, MCProperListRef p_scale)
{
	MCGPoint t_scale;
	if (!MCProperListToScale(p_scale, t_scale))
		return;
	
	MCCanvasTransformScale(x_transform, t_scale.x, t_scale.y);
}

void MCCanvasTransformRotate(MCCanvasTransformRef &x_transform, MCCanvasFloat p_rotation)
{
	MCCanvasTransformConcat(x_transform, MCGAffineTransformMakeRotation(MCCanvasAngleToDegrees(p_rotation)));
}

void MCCanvasTransformTranslate(MCCanvasTransformRef &x_transform, MCCanvasFloat p_dx, MCCanvasFloat p_dy)
{
	MCCanvasTransformConcat(x_transform, MCGAffineTransformMakeTranslation(p_dx, p_dy));
}

void MCCanvasTransformTranslateWithList(MCCanvasTransformRef &x_transform, MCProperListRef p_translation)
{
	MCGPoint t_translation;
	if (!MCProperListToTranslation(p_translation, t_translation))
		return;
	
	MCCanvasTransformTranslate(x_transform, t_translation.x, t_translation.y);
}

void MCCanvasTransformSkew(MCCanvasTransformRef &x_transform, MCCanvasFloat p_xskew, MCCanvasFloat p_yskew)
{
	MCCanvasTransformConcat(x_transform, MCGAffineTransformMakeSkew(p_xskew, p_yskew));
}

void MCCanvasTransformSkew(MCCanvasTransformRef &x_transform, MCProperListRef p_skew)
{
	MCGPoint t_skew;
	if (!MCProperListToSkew(p_skew, t_skew))
		return;
	
	MCCanvasTransformSkew(x_transform, t_skew.x, t_skew.y);
}

////////////////////////////////////////////////////////////////////////////////

// Image

static void __MCCanvasImageDestroy(MCValueRef p_image)
{
	MCImageRepRelease(MCCanvasImageGetImageRep((MCCanvasImageRef) p_image));
}

static bool __MCCanvasImageCopy(MCValueRef p_image, bool p_release, MCValueRef &r_copy)
{
	if (p_release)
		r_copy = p_image;
	else
		r_copy = MCValueRetain(p_image);
	
	return true;
}

static bool __MCCanvasImageEqual(MCValueRef p_left, MCValueRef p_right)
{
	if (p_left == p_right)
		return true;
	
	return MCMemoryCompare(MCValueGetExtraBytesPtr(p_left), MCValueGetExtraBytesPtr(p_right), sizeof(__MCCanvasImageImpl)) == 0;
}

static hash_t __MCCanvasImageHash(MCValueRef p_value)
{
	return MCHashBytes(MCValueGetExtraBytesPtr(p_value), sizeof(__MCCanvasImageImpl));
}

static bool __MCCanvasImageDescribe(MCValueRef p_value, MCStringRef &r_desc)
{
	// TODO - implement describe
	return false;
}

bool MCCanvasImageCreateWithImageRep(MCImageRep *p_image, MCCanvasImageRef &r_image)
{
	bool t_success;
	t_success = true;
	
	MCCanvasImageRef t_image;
	t_image = nil;
	
	if (t_success)
		t_success = MCValueCreateCustom(kMCCanvasImageTypeInfo, sizeof(__MCCanvasImageImpl), t_image);
	
	if (t_success)
	{
		*MCCanvasImageGet(t_image) = MCImageRepRetain(p_image);
		t_success = MCValueInter(t_image, r_image);
	}
	
	MCValueRelease(t_image);
	
	return t_success;
}

__MCCanvasImageImpl *MCCanvasImageGet(MCCanvasImageRef p_image)
{
	return (__MCCanvasImageImpl*)MCValueGetExtraBytesPtr(p_image);
}

MCImageRep *MCCanvasImageGetImageRep(MCCanvasImageRef p_image)
{
	return *MCCanvasImageGet(p_image);
}

// Constructors

void MCCanvasImageMake(MCImageRep *p_image, MCCanvasImageRef &r_image)
{
	/* UNCHECKED */ MCCanvasImageCreateWithImageRep(p_image, r_image);
}

void MCCanvasImageMakeWithPath(MCStringRef p_path, MCCanvasImageRef &r_image)
{
	MCImageRep *t_image_rep;
	t_image_rep = nil;
	
	if (!MCImageRepCreateWithPath(p_path, t_image_rep))
	{
		// TODO - throw image rep error
		return;
	}
	
	MCCanvasImageMake(t_image_rep, r_image);
	MCImageRepRelease(t_image_rep);
}

void MCCanvasImageMakeWithData(MCDataRef p_data, MCCanvasImageRef &r_image)
{
	MCImageRep *t_image_rep;
	t_image_rep = nil;
	
	if (!MCImageRepCreateWithData(p_data, t_image_rep))
	{
		// TODO - throw image rep error
		return;
	}
	
	MCCanvasImageMake(t_image_rep, r_image);
	MCImageRepRelease(t_image_rep);
}

// Input should be unpremultiplied ARGB pixels
void MCCanvasImageMakeWithPixels(integer_t p_width, integer_t p_height, MCDataRef p_pixels, MCCanvasImageRef &r_image)
{
	MCImageRep *t_image_rep;
	t_image_rep = nil;
	
	if (!MCImageRepCreateWithPixels(p_pixels, p_width, p_height, kMCGPixelFormatARGB, false, t_image_rep))
	{
		// TODO - throw image rep error
		return;
	}
	
	MCCanvasImageMake(t_image_rep, r_image);
	MCImageRepRelease(t_image_rep);
}

void MCCanvasImageMakeWithPixelsWithSizeAsList(MCProperListRef p_size, MCDataRef p_pixels, MCCanvasImageRef &r_image)
{
	integer_t t_size[2];
	if (!MCProperListFetchAsArrayOfInteger(p_size, 2, t_size))
	{
		MCCanvasThrowError(kMCCanvasImageSizeListFormatErrorTypeInfo);
		return;
	}
	
	MCCanvasImageMakeWithPixels(t_size[0], t_size[1], p_pixels, r_image);
}

// Properties

void MCCanvasImageGetWidth(MCCanvasImageRef p_image, uint32_t &r_width)
{
	uint32_t t_width, t_height;
	if (!MCImageRepGetGeometry(MCCanvasImageGetImageRep(p_image), t_width, t_height))
	{
		// TODO - throw error
		return;
	}
	r_width = t_width;
}

void MCCanvasImageGetHeight(MCCanvasImageRef p_image, uint32_t &r_height)
{
	uint32_t t_width, t_height;
	if (!MCImageRepGetGeometry(MCCanvasImageGetImageRep(p_image), t_width, t_height))
	{
		// TODO - throw error
		return;
	}
	r_height = t_height;
}

void MCCanvasImageGetPixels(MCCanvasImageRef p_image, MCDataRef &r_pixels)
{
	bool t_success;
	t_success = true;
	
	MCImageRep *t_image_rep;
	t_image_rep = MCCanvasImageGetImageRep(p_image);
	
	MCImageBitmap *t_raster;
	
	// TODO - handle case of missing normal density image
	// TODO - throw appropriate errors
	if (MCImageRepLockRaster(t_image_rep, 0, 1.0, t_raster))
	{
		uint8_t *t_buffer;
		t_buffer = nil;
		
		uint32_t t_buffer_size;
		t_buffer_size = t_raster->height * t_raster->stride;
		
		/* UNCHECKED */ MCMemoryAllocate(t_buffer_size, t_buffer);
		
		uint8_t *t_pixel_row;
		t_pixel_row = t_buffer;
		
		for (uint32_t y = 0; y < t_raster->height; y++)
		{
			uint32_t *t_pixel_ptr;
			t_pixel_ptr = (uint32_t*)t_pixel_row;
			
			for (uint32_t x = 0; x < t_raster->width; x++)
			{
				*t_pixel_ptr = MCGPixelFromNative(kMCGPixelFormatARGB, *t_pixel_ptr);
				t_pixel_ptr++;
			}
			
			t_pixel_row += t_raster->stride;
		}
		
		/* UNCHECKED */ MCDataCreateWithBytesAndRelease(t_buffer, t_buffer_size, r_pixels);
		
		MCImageRepUnlockRaster(t_image_rep, 0, t_raster);
	}
}

void MCCanvasImageGetMetadata(MCCanvasImageRef p_image, MCArrayRef &r_metadata)
{
	// TODO - implement image metadata
}

////////////////////////////////////////////////////////////////////////////////

// Solid Paint

static void __MCCanvasSolidPaintDestroy(MCValueRef p_value)
{
	MCValueRelease(MCCanvasSolidPaintGet((MCCanvasSolidPaintRef)p_value)->color);
}

static bool __MCCanvasSolidPaintCopy(MCValueRef p_value, bool p_release, MCValueRef &r_copy)
{
	if (p_release)
		r_copy = p_value;
	else
		r_copy = MCValueRetain(p_value);
	
	return true;
}

static bool __MCCanvasSolidPaintEqual(MCValueRef p_left, MCValueRef p_right)
{
	if (p_left == p_right)
		return true;
	
	return MCValueIsEqualTo(MCCanvasSolidPaintGet((MCCanvasSolidPaintRef)p_left)->color, MCCanvasSolidPaintGet((MCCanvasSolidPaintRef)p_right)->color);
}

static hash_t __MCCanvasSolidPaintHash(MCValueRef p_value)
{
	return MCValueHash(MCCanvasSolidPaintGet((MCCanvasSolidPaintRef)p_value)->color);
}

static bool __MCCanvasSolidPaintDescribe(MCValueRef p_value, MCStringRef &r_string)
{
	return false;
}

bool MCCanvasSolidPaintCreateWithColor(MCCanvasColorRef p_color, MCCanvasSolidPaintRef &r_paint)
{
	bool t_success;
	t_success = true;
	
	MCCanvasSolidPaintRef t_paint;
	t_paint = nil;
	
	if (t_success)
		t_success = MCValueCreateCustom(kMCCanvasSolidPaintTypeInfo, sizeof(__MCCanvasSolidPaintImpl), t_paint);
	
	if (t_success)
	{
		__MCCanvasSolidPaintImpl *t_impl;
		t_impl = MCCanvasSolidPaintGet(t_paint);
		
		t_impl->color = MCValueRetain(p_color);

		t_success = MCValueInter(t_paint, r_paint);
	}
	
	MCValueRelease(t_paint);
	
	return t_success;
}

__MCCanvasSolidPaintImpl *MCCanvasSolidPaintGet(MCCanvasSolidPaintRef p_paint)
{
	return (__MCCanvasSolidPaintImpl*)MCValueGetExtraBytesPtr(p_paint);
}

bool MCCanvasPaintIsSolidPaint(MCCanvasPaintRef p_paint)
{
	return MCValueGetTypeInfo(p_paint) == kMCCanvasSolidPaintTypeInfo;
}

// Constructor

void MCCanvasSolidPaintMakeWithColor(MCCanvasColorRef p_color, MCCanvasSolidPaintRef &r_paint)
{
	/* UNCHECKED */ MCCanvasSolidPaintCreateWithColor(p_color, r_paint);
}

// Properties

void MCCanvasSolidPaintGetColor(MCCanvasSolidPaintRef p_paint, MCCanvasColorRef &r_color)
{
	r_color = MCValueRetain(MCCanvasSolidPaintGet(p_paint)->color);
}

void MCCanvasSolidPaintSetColor(MCCanvasColorRef p_color, MCCanvasSolidPaintRef &x_paint)
{
	MCCanvasSolidPaintRef t_paint;
	t_paint = nil;
	
	if (!MCCanvasSolidPaintCreateWithColor(p_color, t_paint))
		return;
	
	MCValueAssign(x_paint, t_paint);
	MCValueRelease(t_paint);
}

////////////////////////////////////////////////////////////////////////////////

// Pattern

static void __MCCanvasPatternDestroy(MCValueRef p_value)
{
	MCValueRelease(MCCanvasPatternGet((MCCanvasPatternRef)p_value)->image);
}

static bool __MCCanvasPatternCopy(MCValueRef p_value, bool p_release, MCValueRef &r_copy)
{
	if (p_release)
		r_copy = p_value;
	else
		r_copy = MCValueRetain(p_value);
	
	return true;
}

static bool __MCCanvasPatternEqual(MCValueRef p_left, MCValueRef p_right)
{
	if (p_left == p_right)
		return true;
	
	return MCValueIsEqualTo(MCCanvasPatternGet((MCCanvasPatternRef)p_left)->image, MCCanvasPatternGet((MCCanvasPatternRef)p_right)->image);
}

static hash_t __MCCanvasPatternHash(MCValueRef p_value)
{
	__MCCanvasPatternImpl *t_pattern;
	t_pattern = MCCanvasPatternGet((MCCanvasPatternRef)p_value);
	
	// TODO - ask Mark how to combine hash values
	return MCValueHash(t_pattern->image) ^ MCValueHash(t_pattern->transform);
}

static bool __MCCanvasPatternDescribe(MCValueRef p_value, MCStringRef &r_string)
{
	return false;
}

bool MCCanvasPatternCreateWithImage(MCCanvasImageRef p_image, MCCanvasTransformRef p_transform, MCCanvasPatternRef &r_paint)
{
	bool t_success;
	t_success = true;
	
	MCCanvasPatternRef t_paint;
	t_paint = nil;
	
	if (t_success)
		t_success = MCValueCreateCustom(kMCCanvasPatternTypeInfo, sizeof(__MCCanvasPatternImpl), t_paint);
	
	if (t_success)
	{
		__MCCanvasPatternImpl *t_impl;
		t_impl = MCCanvasPatternGet(t_paint);
		
		t_impl->image = MCValueRetain(p_image);
		t_impl->transform = MCValueRetain(p_transform);
		
		t_success = MCValueInter(t_paint, r_paint);
	}
	
	MCValueRelease(t_paint);
	
	return t_success;
}

__MCCanvasPatternImpl *MCCanvasPatternGet(MCCanvasPatternRef p_paint)
{
	return (__MCCanvasPatternImpl*)MCValueGetExtraBytesPtr(p_paint);
}

bool MCCanvasPaintIsPattern(MCCanvasPaintRef p_paint)
{
	return MCValueGetTypeInfo(p_paint) == kMCCanvasPatternTypeInfo;
}

// Constructor

void MCCanvasPatternMakeWithTransformedImage(MCCanvasImageRef p_image, MCCanvasTransformRef p_transform, MCCanvasPatternRef &r_pattern)
{
	/* UNCHECKED */ MCCanvasPatternCreateWithImage(p_image, p_transform, r_pattern);
}

void MCCanvasPatternMakeWithTransformedImage(MCCanvasImageRef p_image, const MCGAffineTransform &p_transform, MCCanvasPatternRef &r_pattern)
{
	MCCanvasTransformRef t_transform;
	t_transform = nil;
	
	MCCanvasTransformMake(p_transform, t_transform);
	if (!MCErrorIsPending())
		MCCanvasPatternMakeWithTransformedImage(p_image, t_transform, r_pattern);
	MCValueRelease(t_transform);
}

void MCCanvasPatternMakeWithImage(MCCanvasImageRef p_image, MCCanvasPatternRef &r_pattern)
{
	MCCanvasPatternMakeWithTransformedImage(p_image, kMCCanvasIdentityTransform, r_pattern);
}

void MCCanvasPatternMakeWithScaledImage(MCCanvasImageRef p_image, MCCanvasFloat p_xscale, MCCanvasFloat p_yscale, MCCanvasPatternRef &r_pattern)
{
	MCCanvasPatternMakeWithTransformedImage(p_image, MCGAffineTransformMakeScale(p_xscale, p_yscale), r_pattern);
}

void MCCanvasPatternMakeWithImageScaledWithList(MCCanvasImageRef p_image, MCProperListRef p_scale, MCCanvasPatternRef &r_pattern)
{
	MCGPoint t_scale;
	if (!MCProperListToScale(p_scale, t_scale))
		return;
	
	MCCanvasPatternMakeWithScaledImage(p_image, t_scale.x, t_scale.y, r_pattern);
}

void MCCanvasPatternMakeWithRotatedImage(MCCanvasImageRef p_image, MCCanvasFloat p_angle, MCCanvasPatternRef &r_pattern)
{
	MCCanvasPatternMakeWithTransformedImage(p_image, MCGAffineTransformMakeRotation(MCCanvasAngleToDegrees(p_angle)), r_pattern);
}

void MCCanvasPatternMakeWithTranslatedImage(MCCanvasImageRef p_image, MCCanvasFloat p_x, MCCanvasFloat p_y, MCCanvasPatternRef &r_pattern)
{
	MCCanvasPatternMakeWithTransformedImage(p_image, MCGAffineTransformMakeTranslation(p_x, p_y), r_pattern);
}

void MCCanvasPatternMakeWithImageTranslatedWithList(MCCanvasImageRef p_image, MCProperListRef p_translation, MCCanvasPatternRef &r_pattern)
{
	MCGPoint t_translation;
	if (!MCProperListToTranslation(p_translation, t_translation))
		return;
	
	MCCanvasPatternMakeWithTranslatedImage(p_image, t_translation.x, t_translation.y, r_pattern);
}

// Properties

void MCCanvasPatternSet(MCCanvasImageRef p_image, MCCanvasTransformRef p_transform, MCCanvasPatternRef &x_pattern)
{
	MCCanvasPatternRef t_pattern;
	if (!MCCanvasPatternCreateWithImage(p_image, p_transform, t_pattern))
		return;
	
	MCValueAssign(x_pattern, t_pattern);
	MCValueRelease(t_pattern);
}

void MCCanvasPatternGetImage(MCCanvasPatternRef p_pattern, MCCanvasImageRef &r_image)
{
	r_image = MCValueRetain(MCCanvasPatternGet(p_pattern)->image);
}

void MCCanvasPatternSetImage(MCCanvasImageRef p_image, MCCanvasPatternRef &x_pattern)
{
	MCCanvasPatternSet(p_image, MCCanvasPatternGet(x_pattern)->transform, x_pattern);
}

void MCCanvasPatternGetTransform(MCCanvasPatternRef p_pattern, MCCanvasTransformRef &r_transform)
{
	r_transform = MCValueRetain(MCCanvasPatternGet(p_pattern)->transform);
}

void MCCanvasPatternSetTransform(MCCanvasTransformRef p_transform, MCCanvasPatternRef &x_pattern)
{
	MCCanvasPatternSet(MCCanvasPatternGet(x_pattern)->image, p_transform, x_pattern);
}

// Operators

void MCCanvasPatternTransform(MCCanvasPatternRef &x_pattern, const MCGAffineTransform &p_transform)
{
	MCCanvasTransformRef t_transform;
	t_transform = MCValueRetain(MCCanvasPatternGet(x_pattern)->transform);
	
	MCCanvasTransformConcat(t_transform, p_transform);
	
	if (!MCErrorIsPending())
		MCCanvasPatternSetTransform(t_transform, x_pattern);
	
	MCValueRelease(t_transform);
}

void MCCanvasPatternTransform(MCCanvasPatternRef &x_pattern, MCCanvasTransformRef p_transform)
{
	MCCanvasPatternTransform(x_pattern, *MCCanvasTransformGet(p_transform));
}

void MCCanvasPatternScale(MCCanvasPatternRef &x_pattern, MCCanvasFloat p_xscale, MCCanvasFloat p_yscale)
{
	MCCanvasPatternTransform(x_pattern, MCGAffineTransformMakeScale(p_xscale, p_yscale));
}

void MCCanvasPatternScaleWithList(MCCanvasPatternRef &x_pattern, MCProperListRef p_scale)
{
	MCGPoint t_scale;
	if (!MCProperListToScale(p_scale, t_scale))
		return;
	
	MCCanvasPatternScale(x_pattern, t_scale.x, t_scale.y);
}

void MCCanvasPatternRotate(MCCanvasPatternRef &x_pattern, MCCanvasFloat p_angle)
{
	MCCanvasPatternTransform(x_pattern, MCGAffineTransformMakeRotation(MCCanvasAngleToDegrees(p_angle)));
}

void MCCanvasPatternTranslate(MCCanvasPatternRef &x_pattern, MCCanvasFloat p_x, MCCanvasFloat p_y)
{
	MCCanvasPatternTransform(x_pattern, MCGAffineTransformMakeTranslation(p_x, p_y));
}

void MCCanvasPatternTranslateWithList(MCCanvasPatternRef &x_pattern, MCProperListRef p_translation)
{
	MCGPoint t_translation;
	if (!MCProperListToTranslation(p_translation, t_translation))
		return;
	
	MCCanvasPatternTranslate(x_pattern, t_translation.x, t_translation.y);
}

////////////////////////////////////////////////////////////////////////////////

// Gradient Stop

static void __MCCanvasGradientStopDestroy(MCValueRef p_stop)
{
	MCValueRelease(MCCanvasGradientStopGet((MCCanvasGradientStopRef)p_stop)->color);
}

static bool __MCCanvasGradientStopCopy(MCValueRef p_value, bool p_release, MCValueRef &r_copy)
{
	if (p_release)
		r_copy = p_value;
	else
		r_copy = MCValueRetain(p_value);
	
	return true;
}

static bool __MCCanvasGradientStopEqual(MCValueRef p_left, MCValueRef p_right)
{
	if (p_left == p_right)
		return true;
	
	__MCCanvasGradientStopImpl *t_left, *t_right;
	t_left = (__MCCanvasGradientStopImpl*)MCValueGetExtraBytesPtr(p_left);
	t_right = (__MCCanvasGradientStopImpl*)MCValueGetExtraBytesPtr(p_right);
	
	return t_left->offset == t_right->offset && MCValueIsEqualTo(t_left->color, t_right->color);
}

static hash_t __MCCanvasGradientStopHash(MCValueRef p_value)
{
	__MCCanvasGradientStopImpl *t_stop;
	t_stop = (__MCCanvasGradientStopImpl*)MCValueGetExtraBytesPtr(p_value);

	return MCValueHash(t_stop->color) ^ MCHashDouble(t_stop->offset);
}

static bool __MCCanvasGradientStopDescribe(MCValueRef p_value, MCStringRef &r_desc)
{
	// TODO - implement describe
	return false;
}

bool MCCanvasGradientStopCreate(MCCanvasFloat p_offset, MCCanvasColorRef p_color, MCCanvasGradientStopRef &r_stop)
{
	bool t_success;
	t_success = true;
	
	MCCanvasGradientStopRef t_stop;
	t_stop = nil;
	
	if (t_success)
		t_success = MCValueCreateCustom(kMCCanvasGradientStopTypeInfo, sizeof(__MCCanvasGradientStopImpl), t_stop);
	
	if (t_success)
	{
		__MCCanvasGradientStopImpl *t_impl;
		t_impl = MCCanvasGradientStopGet(t_stop);
		t_impl->offset = p_offset;
		t_impl->color = MCValueRetain(p_color);
		t_success = MCValueInter(t_stop, r_stop);
	}
	
	MCValueRelease(t_stop);
	
	return t_success;
}

__MCCanvasGradientStopImpl *MCCanvasGradientStopGet(MCCanvasGradientStopRef p_stop)
{
	return (__MCCanvasGradientStopImpl*)MCValueGetExtraBytesPtr(p_stop);
}

// Constructors

void MCCanvasGradientStopMake(MCCanvasFloat p_offset, MCCanvasColorRef p_color, MCCanvasGradientStopRef &r_stop)
{
	/* UNCHECKED */ MCCanvasGradientStopCreate(p_offset, p_color, r_stop);
}

//	Properties

void MCCanvasGradientStopSet(MCCanvasFloat p_offset, MCCanvasColorRef p_color, MCCanvasGradientStopRef &x_stop)
{
	MCCanvasGradientStopRef t_stop;
	if (!MCCanvasGradientStopCreate(p_offset, p_color, t_stop))
		return;
	MCValueAssign(x_stop, t_stop);
	MCValueRelease(t_stop);
}

void MCCanvasGradientStopGetOffset(MCCanvasGradientStopRef p_stop, MCCanvasFloat &r_offset)
{
	r_offset = MCCanvasGradientStopGet(p_stop)->offset;
}

void MCCanvasGradientStopSetOffset(MCCanvasFloat p_offset, MCCanvasGradientStopRef &x_stop)
{
	__MCCanvasGradientStopImpl *t_stop;
	t_stop = MCCanvasGradientStopGet(x_stop);
	
	MCCanvasGradientStopSet(p_offset, t_stop->color, x_stop);
}

void MCCanvasGradientStopGetColor(MCCanvasGradientStopRef p_stop, MCCanvasColorRef &r_color)
{
	r_color = MCValueRetain(MCCanvasGradientStopGet(p_stop)->color);
}

void MCCanvasGradientStopSetColor(MCCanvasColorRef p_color, MCCanvasGradientStopRef &x_stop)
{
	__MCCanvasGradientStopImpl *t_stop;
	t_stop = MCCanvasGradientStopGet(x_stop);
	
	MCCanvasGradientStopSet(t_stop->offset, p_color, x_stop);
}

// Gradient

static void __MCCanvasGradientDestroy(MCValueRef p_value)
{
	__MCCanvasGradientImpl *t_gradient;
	t_gradient = MCCanvasGradientGet((MCCanvasGradientRef)p_value);
	
	MCValueRelease(t_gradient->ramp);
	MCValueRelease(t_gradient->transform);
}

static bool __MCCanvasGradientCopy(MCValueRef p_value, bool p_release, MCValueRef &r_copy)
{
	if (p_release)
		r_copy = p_value;
	else
		r_copy = MCValueRetain(p_value);
	
	return true;
}

static bool __MCCanvasGradientEqual(MCValueRef p_left, MCValueRef p_right)
{
	if (p_left == p_right)
		return true;
	
	__MCCanvasGradientImpl *t_left, *t_right;
	t_left = MCCanvasGradientGet((MCCanvasGradientRef)p_left);
	t_right = MCCanvasGradientGet((MCCanvasGradientRef)p_right);
	
	return t_left->function == t_right->function &&
	MCValueIsEqualTo(t_left->ramp, t_right->ramp) &&
	t_left->mirror == t_right->mirror &&
	t_left->wrap == t_right->wrap &&
	t_left->repeats == t_right->repeats &&
	MCValueIsEqualTo(t_left->transform, t_right->transform) &&
	t_left->filter == t_right->filter;
}

static hash_t __MCCanvasGradientHash(MCValueRef p_value)
{
	__MCCanvasGradientImpl *t_gradient;
	t_gradient = MCCanvasGradientGet((MCCanvasGradientRef)p_value);
	// TODO - ask Mark how to combine hash values
	return MCHashInteger(t_gradient->mirror | (t_gradient->wrap < 1)) ^ MCHashInteger(t_gradient->filter) ^ MCValueHash(t_gradient->ramp) ^ MCHashInteger(t_gradient->repeats) ^ MCValueHash(t_gradient->transform) ^ MCHashInteger(t_gradient->filter);
}

static bool __MCCanvasGradientDescribe(MCValueRef p_value, MCStringRef &r_string)
{
	return false;
}

bool MCCanvasGradientCreate(const __MCCanvasGradientImpl &p_gradient, MCCanvasGradientRef &r_paint)
{
	bool t_success;
	t_success = true;
	
	MCCanvasGradientRef t_paint;
	t_paint = nil;
	
	if (t_success)
		t_success = MCValueCreateCustom(kMCCanvasGradientTypeInfo, sizeof(__MCCanvasGradientImpl), t_paint);
	
	if (t_success)
	{
		__MCCanvasGradientImpl *t_impl;
		t_impl = MCCanvasGradientGet(t_paint);
		
		*t_impl = p_gradient;
		MCValueRetain(t_impl->ramp);
		MCValueRetain(t_impl->transform);
		
		t_success = MCValueInter(t_paint, r_paint);
	}
	
	MCValueRelease(t_paint);
	
	return t_success;
}

__MCCanvasGradientImpl *MCCanvasGradientGet(MCCanvasGradientRef p_paint)
{
	return (__MCCanvasGradientImpl*)MCValueGetExtraBytesPtr(p_paint);
}

bool MCCanvasPaintIsGradient(MCCanvasPaintRef p_paint)
{
	return MCValueGetTypeInfo(p_paint) == kMCCanvasGradientTypeInfo;
}

// Gradient

bool MCProperListFetchGradientStopAt(MCProperListRef p_list, uindex_t p_index, MCCanvasGradientStopRef &r_stop)
{
	if (p_index >= MCProperListGetLength(p_list))
		return false;
	
	MCValueRef t_value;
	t_value = MCProperListFetchElementAtIndex(p_list, p_index);
	
	if (MCValueGetTypeInfo(t_value) != kMCCanvasGradientStopTypeInfo)
		return false;
	
	r_stop = (MCCanvasGradientStopRef)t_value;
	
	return true;
}

bool MCCanvasGradientCheckStopOrder(MCProperListRef p_ramp)
{
	uindex_t t_length;
	t_length = MCProperListGetLength(p_ramp);

	if (t_length == 0)
		return true;
	
	MCCanvasGradientStopRef t_prev_stop;
	if (!MCProperListFetchGradientStopAt(p_ramp, 0, t_prev_stop))
		return false;
	
	for (uint32_t i = 1; i < t_length; i++)
	{
		MCCanvasGradientStopRef t_stop;
		if (!MCProperListFetchGradientStopAt(p_ramp, i, t_stop))
			return false;
		
		if (MCCanvasGradientStopGet(t_stop)->offset < MCCanvasGradientStopGet(t_prev_stop)->offset)
		{
			// TODO - throw stop offset order error
			return false;
		}
	}
	
	return true;
}

//////////

void MCCanvasGradientEvaluateType(integer_t p_type, integer_t& r_type)
{
    r_type = p_type;
}

// Constructor

void MCCanvasGradientMakeWithRamp(integer_t p_type, MCProperListRef p_ramp, MCCanvasGradientRef &r_gradient)
{
	MCCanvasGradientRef t_gradient;
	
	if (!MCCanvasGradientCheckStopOrder(p_ramp))
	{
		return;
	}
	
	__MCCanvasGradientImpl t_gradient_impl;
	MCMemoryClear(&t_gradient_impl, sizeof(__MCCanvasGradientImpl));
	t_gradient_impl.function = (MCGGradientFunction)p_type;
	t_gradient_impl.mirror = false;
	t_gradient_impl.wrap = false;
	t_gradient_impl.repeats = 1;
	t_gradient_impl.transform = kMCCanvasIdentityTransform;
	t_gradient_impl.filter = kMCGImageFilterNone;
    t_gradient_impl.ramp = p_ramp;
	
	/* UNCHECKED */ MCCanvasGradientCreate(t_gradient_impl, r_gradient);
}

// Properties

void MCCanvasGradientSet(const __MCCanvasGradientImpl &p_gradient, MCCanvasGradientRef &x_gradient)
{
	MCCanvasGradientRef t_gradient;
	if (!MCCanvasGradientCreate(p_gradient, t_gradient))
		return;
	
	MCValueAssign(x_gradient, t_gradient);
	MCValueRelease(t_gradient);
}

void MCCanvasGradientGetRamp(MCCanvasGradientRef p_gradient, MCProperListRef &r_ramp)
{
	r_ramp = MCValueRetain(MCCanvasGradientGet(p_gradient)->ramp);
}

void MCCanvasGradientSetRamp(MCProperListRef p_ramp, MCCanvasGradientRef &x_gradient)
{
	if (!MCCanvasGradientCheckStopOrder(p_ramp))
		return;
	
	__MCCanvasGradientImpl t_gradient;
	t_gradient = *MCCanvasGradientGet(x_gradient);
	
	t_gradient.ramp = p_ramp;
	
	MCCanvasGradientSet(t_gradient, x_gradient);
}

void MCCanvasGradientGetTypeAsString(MCCanvasGradientRef p_gradient, MCStringRef &r_string)
{
	/* UNCHECKED */ MCCanvasGradientTypeToString(MCCanvasGradientGet(p_gradient)->function, r_string);
}

void MCCanvasGradientSetTypeAsString(MCStringRef p_string, MCCanvasGradientRef &x_gradient)
{
	__MCCanvasGradientImpl t_gradient;
	t_gradient = *MCCanvasGradientGet(x_gradient);
	
	if (!MCCanvasGradientTypeFromString(p_string, t_gradient.function))
	{
		// TODO - throw gradient type error
		return;
	}
	
	MCCanvasGradientSet(t_gradient, x_gradient);
}

void MCCanvasGradientGetRepeat(MCCanvasGradientRef p_gradient, integer_t &r_repeat)
{
	r_repeat = MCCanvasGradientGet(p_gradient)->repeats;
}

void MCCanvasGradientSetRepeat(integer_t p_repeat, MCCanvasGradientRef &x_gradient)
{
	__MCCanvasGradientImpl t_gradient;
	t_gradient = *MCCanvasGradientGet(x_gradient);
	
	t_gradient.repeats = p_repeat;
	
	MCCanvasGradientSet(t_gradient, x_gradient);
}

void MCCanvasGradientGetWrap(MCCanvasGradientRef p_gradient, bool &r_wrap)
{
	r_wrap = MCCanvasGradientGet(p_gradient)->wrap;
}

void MCCanvasGradientSetWrap(bool p_wrap, MCCanvasGradientRef &x_gradient)
{
	__MCCanvasGradientImpl t_gradient;
	t_gradient = *MCCanvasGradientGet(x_gradient);
	
	t_gradient.wrap = p_wrap;
	
	MCCanvasGradientSet(t_gradient, x_gradient);
}

void MCCanvasGradientGetMirror(MCCanvasGradientRef p_gradient, bool &r_mirror)
{
	r_mirror = MCCanvasGradientGet(p_gradient)->mirror;
}

void MCCanvasGradientSetMirror(bool p_mirror, MCCanvasGradientRef &x_gradient)
{
	__MCCanvasGradientImpl t_gradient;
	t_gradient = *MCCanvasGradientGet(x_gradient);
	
	t_gradient.mirror = p_mirror;
	
	MCCanvasGradientSet(t_gradient, x_gradient);
}

void MCCanvasGradientTransformToPoints(const MCGAffineTransform &p_transform, MCGPoint &r_from, MCGPoint &r_to, MCGPoint &r_via)
{
	r_from = MCGPointApplyAffineTransform(MCGPointMake(0, 0), p_transform);
	r_to = MCGPointApplyAffineTransform(MCGPointMake(0, 1), p_transform);
	r_via = MCGPointApplyAffineTransform(MCGPointMake(1, 0), p_transform);
}

bool MCCanvasGradientTransformFromPoints(const MCGPoint &p_from, const MCGPoint &p_to, const MCGPoint &p_via, MCGAffineTransform &r_transform)
{
	MCGPoint t_src[3], t_dst[3];
	t_src[0] = MCGPointMake(0, 0);
	t_src[1] = MCGPointMake(0, 1);
	t_src[2] = MCGPointMake(1, 0);
	
	t_dst[0] = p_from;
	t_dst[1] = p_to;
	t_dst[2] = p_via;
	
	return MCGAffineTransformFromPoints(t_src, t_dst, r_transform);
}

void MCCanvasGradientGetTransform(MCCanvasGradientRef p_gradient, MCGAffineTransform &r_transform)
{
	r_transform = *MCCanvasTransformGet(MCCanvasGradientGet(p_gradient)->transform);
}

void MCCanvasGradientSetTransform(MCCanvasGradientRef &x_gradient, const MCGAffineTransform &p_transform)
{
	MCCanvasTransformRef t_transform;
	t_transform = nil;
	
	MCCanvasTransformMake(p_transform, t_transform);
	if (!MCErrorIsPending())
		MCCanvasGradientSetTransform(t_transform, x_gradient);
	MCValueRelease(t_transform);
}

void MCCanvasGradientGetPoints(MCCanvasGradientRef p_gradient, MCGPoint &r_from, MCGPoint &r_to, MCGPoint &r_via)
{
	MCGAffineTransform t_transform;
	MCCanvasGradientGetTransform(p_gradient, t_transform);
	MCCanvasGradientTransformToPoints(t_transform, r_from, r_to, r_via);
}

void MCCanvasGradientSetPoints(MCCanvasGradientRef &x_gradient, const MCGPoint &p_from, const MCGPoint &p_to, const MCGPoint &p_via)
{
	MCGAffineTransform t_transform;
	if (!MCCanvasGradientTransformFromPoints(p_from, p_to, p_via, t_transform))
	{
		// TODO - throw error
	}
	MCCanvasGradientSetTransform(x_gradient, t_transform);
}

void MCCanvasGradientGetFrom(MCCanvasGradientRef p_gradient, MCCanvasPointRef &r_from)
{
	MCGPoint t_from, t_to, t_via;
	MCCanvasGradientGetPoints(p_gradient, t_from, t_to, t_via);
	
	/* UNCHECKED */ MCCanvasPointCreateWithMCGPoint(t_from, r_from);
}

void MCCanvasGradientGetTo(MCCanvasGradientRef p_gradient, MCCanvasPointRef &r_to)
{
	MCGPoint t_from, t_to, t_via;
	MCCanvasGradientGetPoints(p_gradient, t_from, t_to, t_via);
	
	/* UNCHECKED */ MCCanvasPointCreateWithMCGPoint(t_to, r_to);
}

void MCCanvasGradientGetVia(MCCanvasGradientRef p_gradient, MCCanvasPointRef &r_via)
{
	MCGPoint t_from, t_to, t_via;
	MCCanvasGradientGetPoints(p_gradient, t_from, t_to, t_via);
	
	/* UNCHECKED */ MCCanvasPointCreateWithMCGPoint(t_via, r_via);
}

void MCCanvasGradientSetFrom(MCCanvasPointRef p_from, MCCanvasGradientRef &x_gradient)
{
	MCGPoint t_from, t_to, t_via;
	MCCanvasGradientGetPoints(x_gradient, t_from, t_to, t_via);
	MCCanvasGradientSetPoints(x_gradient, *MCCanvasPointGet(p_from), t_to, t_via);
}

void MCCanvasGradientSetTo(MCCanvasPointRef p_to, MCCanvasGradientRef &x_gradient)
{
	MCGPoint t_from, t_to, t_via;
	MCCanvasGradientGetPoints(x_gradient, t_from, t_to, t_via);
	MCCanvasGradientSetPoints(x_gradient, t_from, *MCCanvasPointGet(p_to), t_via);
}

void MCCanvasGradientSetVia(MCCanvasPointRef p_via, MCCanvasGradientRef &x_gradient)
{
	MCGPoint t_from, t_to, t_via;
	MCCanvasGradientGetPoints(x_gradient, t_from, t_to, t_via);
	MCCanvasGradientSetPoints(x_gradient, t_from, t_to, *MCCanvasPointGet(p_via));
}

void MCCanvasGradientGetTransform(MCCanvasGradientRef p_gradient, MCCanvasTransformRef &r_transform)
{
	r_transform = MCValueRetain(MCCanvasGradientGet(p_gradient)->transform);
}

void MCCanvasGradientSetTransform(MCCanvasTransformRef p_transform, MCCanvasGradientRef &x_gradient)
{
	__MCCanvasGradientImpl t_gradient;
	t_gradient = *MCCanvasGradientGet(x_gradient);
	t_gradient.transform = p_transform;
	MCCanvasGradientSet(t_gradient, x_gradient);
}

// Operators

// TODO - replace this with a binary search :)
bool MCProperListGetGradientStopInsertionPoint(MCProperListRef p_list, MCCanvasGradientStopRef p_stop, uindex_t &r_index)
{
	uindex_t t_length;
	t_length = MCProperListGetLength(p_list);
	
	MCCanvasFloat t_offset;
	t_offset = MCCanvasGradientStopGet(p_stop)->offset;
	
	for (uindex_t i = 0; i < t_length; i++)
	{
		MCCanvasGradientStopRef t_stop;
		if (!MCProperListFetchGradientStopAt(p_list, i, t_stop))
			return false;
		if (t_offset < MCCanvasGradientStopGet(p_stop)->offset)
		{
			r_index = i;
			return true;
		}
	}
	
	r_index = t_length;
	
	return true;
}

void MCCanvasGradientAddStop(MCCanvasGradientStopRef p_stop, MCCanvasGradientRef &x_gradient)
{
	bool t_success;
	t_success = true;
	
	__MCCanvasGradientStopImpl *t_new_stop;
	t_new_stop = MCCanvasGradientStopGet(p_stop);
	
	if (t_new_stop->offset < 0 || t_new_stop->offset > 1)
	{
		// TODO - throw offset range error
		return;
	}
	
	__MCCanvasGradientImpl t_gradient;
	t_gradient = *MCCanvasGradientGet(x_gradient);
	
	MCProperListRef t_mutable_ramp;
	t_mutable_ramp = nil;
	
	if (t_success)
		t_success = MCProperListMutableCopy(t_gradient.ramp, t_mutable_ramp);
	
	uindex_t t_index;
	
	if (t_success)
		t_success = MCProperListGetGradientStopInsertionPoint(t_mutable_ramp, p_stop, t_index);
	
	if (t_success)
		t_success = MCProperListInsertElement(t_mutable_ramp, p_stop, t_index);
	
	MCProperListRef t_new_ramp;
	t_new_ramp = nil;
	
	if (t_success)
		t_success = MCProperListCopyAndRelease(t_mutable_ramp, t_new_ramp);
	
	if (t_success)
	{
		t_gradient.ramp = t_new_ramp;
		MCCanvasGradientSet(t_gradient, x_gradient);
		MCValueRelease(t_new_ramp);
	}
	else
		MCValueRelease(t_mutable_ramp);
}

void MCCanvasGradientTransform(MCCanvasGradientRef &x_gradient, const MCGAffineTransform &p_transform)
{
	MCCanvasTransformRef t_transform;
	t_transform = MCValueRetain(MCCanvasGradientGet(x_gradient)->transform);
	
	MCCanvasTransformConcat(t_transform, p_transform);
	
	if (!MCErrorIsPending())
		MCCanvasGradientSetTransform(t_transform, x_gradient);
	
	MCValueRelease(t_transform);
}

void MCCanvasGradientTransform(MCCanvasGradientRef &x_gradient, MCCanvasTransformRef p_transform)
{
	MCCanvasGradientTransform(x_gradient, *MCCanvasTransformGet(p_transform));
}

void MCCanvasGradientScale(MCCanvasGradientRef &x_gradient, MCCanvasFloat p_xscale, MCCanvasFloat p_yscale)
{
	MCCanvasGradientTransform(x_gradient, MCGAffineTransformMakeScale(p_xscale, p_yscale));
}

void MCCanvasGradientScaleWithList(MCCanvasGradientRef &x_gradient, MCProperListRef p_scale)
{
	MCGPoint t_scale;
	if (!MCProperListToScale(p_scale, t_scale))
		return;
	
	MCCanvasGradientScale(x_gradient, t_scale.x, t_scale.y);
}

void MCCanvasGradientRotate(MCCanvasGradientRef &x_gradient, MCCanvasFloat p_angle)
{
	MCCanvasGradientTransform(x_gradient, MCGAffineTransformMakeRotation(MCCanvasAngleToDegrees(p_angle)));
}

void MCCanvasGradientTranslate(MCCanvasGradientRef &x_gradient, MCCanvasFloat p_x, MCCanvasFloat p_y)
{
	MCCanvasGradientTransform(x_gradient, MCGAffineTransformMakeTranslation(p_x, p_y));
}

void MCCanvasGradientTranslateWithList(MCCanvasGradientRef &x_gradient, MCProperListRef p_translation)
{
	MCGPoint t_translation;
	if (!MCProperListToTranslation(p_translation, t_translation))
		return;
	
	MCCanvasGradientTranslate(x_gradient, t_translation.x, t_translation.y);
}

////////////////////////////////////////////////////////////////////////////////

// Path

static void __MCCanvasPathDestroy(MCValueRef p_value)
{
	MCGPathRelease(MCCanvasPathGetMCGPath((MCCanvasPathRef) p_value));
}

static bool __MCCanvasPathCopy(MCValueRef p_value, bool p_release, MCValueRef &r_copy)
{
	if (p_release)
		r_copy = p_value;
	else
		r_copy = MCValueRetain(p_value);
	
	return true;
}

static bool __MCCanvasPathEqual(MCValueRef p_left, MCValueRef p_right)
{
	if (p_left == p_right)
		return true;
	
	return MCGPathIsEqualTo(MCCanvasPathGetMCGPath((MCCanvasPathRef)p_left), MCCanvasPathGetMCGPath((MCCanvasPathRef)p_right));
}

static bool __MCCanvasPathHashCallback(void *p_context, MCGPathCommand p_command, MCGPoint *p_points, uint32_t p_point_count)
{
	hash_t *t_hash;
	t_hash = static_cast<hash_t*>(p_context);
	
	*t_hash ^= MCHashInteger(p_command);
	for (uint32_t i = 0; i < p_point_count; i++)
	{
		*t_hash ^= MCHashDouble(p_points[i].x);
		*t_hash ^= MCHashDouble(p_points[i].y);
	}
	
	return true;
}

static hash_t __MCCanvasPathHash(MCValueRef p_value)
{
	hash_t t_hash;
	t_hash = 0;
	
	/* UNCHECKED */ MCGPathIterate(MCCanvasPathGetMCGPath((MCCanvasPathRef)p_value), __MCCanvasPathHashCallback, &t_hash);
	
	return t_hash;
}

static bool __MCCanvasPathDescribe(MCValueRef p_value, MCStringRef &r_desc)
{
	// TODO - implement describe
	return false;
}

bool MCCanvasPathCreateWithMCGPath(MCGPathRef p_path, MCCanvasPathRef &r_path)
{
	bool t_success;
	t_success = true;
	
	MCCanvasPathRef t_path;
	t_path = nil;
	
	if (t_success)
		t_success = MCValueCreateCustom(kMCCanvasPathTypeInfo, sizeof(__MCCanvasPathImpl), t_path);
	
	if (t_success)
	{
		MCGPathCopy(p_path, *MCCanvasPathGet(t_path));
		t_success = MCGPathIsValid(*MCCanvasPathGet(t_path));
	}
	
	if (t_success)
		t_success = MCValueInter(t_path, r_path);
	
	MCValueRelease(t_path);
	
	return t_success;
}

__MCCanvasPathImpl *MCCanvasPathGet(MCCanvasPathRef p_path)
{
	return (__MCCanvasPathImpl*)MCValueGetExtraBytesPtr(p_path);
}

MCGPathRef MCCanvasPathGetMCGPath(MCCanvasPathRef p_path)
{
	return *MCCanvasPathGet(p_path);
}

//////////

bool MCProperListToRadii(MCProperListRef p_list, MCGPoint &r_radii)
{
	bool t_success;
	t_success = true;
	
	real64_t t_radii[2];
	
	if (t_success)
		t_success = MCProperListFetchAsArrayOfReal(p_list, 2, t_radii);
	
	if (t_success)
		r_radii = MCGPointMake(t_radii[0], t_radii[1]);
	else
		MCCanvasThrowError(kMCCanvasRadiiListFormatErrorTypeInfo);
	
	return t_success;
}

bool MCCanvasPathParseInstructions(MCStringRef p_instructions, MCGPathIterateCallback p_callback, void *p_context);
bool MCCanvasPathUnparseInstructions(MCCanvasPathRef &p_path, MCStringRef &r_string);

// Constructors

bool MCCanvasPathMakeWithInstructionsCallback(void *p_context, MCGPathCommand p_command, MCGPoint *p_points, uint32_t p_point_count)
{
	MCGPathRef t_path;
	t_path = static_cast<MCGPathRef>(p_context);
	
	switch (p_command)
	{
		case kMCGPathCommandMoveTo:
			MCGPathMoveTo(t_path, p_points[0]);
			break;
			
		case kMCGPathCommandLineTo:
			MCGPathLineTo(t_path, p_points[0]);
			break;
			
		case kMCGPathCommandQuadCurveTo:
			MCGPathQuadraticTo(t_path, p_points[0], p_points[1]);
			break;
			
		case kMCGPathCommandCubicCurveTo:
			MCGPathCubicTo(t_path, p_points[0], p_points[1], p_points[2]);
			break;
			
		case kMCGPathCommandCloseSubpath:
			MCGPathCloseSubpath(t_path);
			break;
			
		case kMCGPathCommandEnd:
			break;
			
		default:
			MCAssert(false);
	}
	
	return MCGPathIsValid(t_path);
}

void MCCanvasPathMakeWithMCGPath(MCGPathRef p_path, MCCanvasPathRef &r_path)
{
	/* UNCHECKED */ MCCanvasPathCreateWithMCGPath(p_path, r_path);
}

// TODO - investigate error handling in libgraphics, libskia - don't think skia mem errors are tested for
void MCCanvasPathMakeWithInstructionsAsString(MCStringRef p_instructions, MCCanvasPathRef &r_path)
{
	bool t_success;
	t_success = true;
	
	MCGPathRef t_path;
	t_path = nil;
	
	if (t_success)
		t_success = MCGPathCreateMutable(t_path);
	
	if (t_success)
		t_success = MCCanvasPathParseInstructions(p_instructions, MCCanvasPathMakeWithInstructionsCallback, t_path);
	
	if (t_success)
		MCCanvasPathMakeWithMCGPath(t_path, r_path);
	
	MCGPathRelease(t_path);
}

void MCCanvasPathMakeWithRoundedRectangleWithRadii(MCCanvasRectangleRef p_rect, MCCanvasFloat p_x_radius, MCCanvasFloat p_y_radius, MCCanvasPathRef &r_path)
{
	MCGPathRef t_path;
	t_path = nil;
	
	if (!MCGPathCreateMutable(t_path))
		return;
	
	MCGPathAddRoundedRectangle(t_path, *MCCanvasRectangleGet(p_rect), MCGSizeMake(p_x_radius, p_y_radius));
	if (MCGPathIsValid(t_path))
		MCCanvasPathMakeWithMCGPath(t_path, r_path);
	
	MCGPathRelease(t_path);
}

void MCCanvasPathMakeWithRoundedRectangle(MCCanvasRectangleRef p_rect, MCCanvasFloat p_radius, MCCanvasPathRef &r_path)
{
	MCCanvasPathMakeWithRoundedRectangleWithRadii(p_rect, p_radius, p_radius, r_path);
}

void MCCanvasPathMakeWithRoundedRectangleWithRadiiAsList(MCCanvasRectangleRef p_rect, MCProperListRef p_radii, MCCanvasPathRef &r_path)
{
	MCGPoint t_radii;
	if (!MCProperListToRadii(p_radii, t_radii))
		return;
	
	MCCanvasPathMakeWithRoundedRectangleWithRadii(p_rect, t_radii.x, t_radii.y, r_path);
}

void MCCanvasPathMakeWithRectangle(MCCanvasRectangleRef p_rect, MCCanvasPathRef &r_path)
{
	MCGPathRef t_path;
	t_path = nil;
	
	if (!MCGPathCreateMutable(t_path))
		return;
	
	MCGPathAddRectangle(t_path, *MCCanvasRectangleGet(p_rect));
	if (MCGPathIsValid(t_path))
		MCCanvasPathMakeWithMCGPath(t_path, r_path);

	MCGPathRelease(t_path);
}

void MCCanvasPathMakeWithEllipse(MCCanvasPointRef p_center, MCCanvasFloat p_radius_x, MCCanvasFloat p_radius_y, MCCanvasPathRef &r_path)
{
	MCGPathRef t_path;
	t_path = nil;
	
	if (!MCGPathCreateMutable(t_path))
		return;
	
	MCGPathAddEllipse(t_path, *MCCanvasPointGet(p_center), MCGSizeMake(2*p_radius_x, 2*p_radius_y), 0);
	if (MCGPathIsValid(t_path))
		MCCanvasPathMakeWithMCGPath(t_path, r_path);
	
	MCGPathRelease(t_path);
}

void MCCanvasPathMakeWithEllipseWithRadiiAsList(MCCanvasPointRef p_center, MCProperListRef p_radii, MCCanvasPathRef &r_path)
{
	MCGPoint t_radii;
	if (!MCProperListToRadii(p_radii, t_radii))
		return;
	
	MCCanvasPathMakeWithEllipse(p_center, t_radii.x, t_radii.y, r_path);
}

void MCCanvasPathMakeWithCircle(MCCanvasPointRef p_center, MCCanvasFloat p_radius, MCCanvasPathRef &r_path)
{
	MCCanvasPathMakeWithEllipse(p_center, p_radius, p_radius, r_path);
}

void MCCanvasPathMakeWithLine(MCCanvasPointRef p_start, MCCanvasPointRef p_end, MCCanvasPathRef &r_path)
{
	MCGPathRef t_path;
	t_path = nil;
	
	if (!MCGPathCreateMutable(t_path))
		return;
	
	MCGPathAddLine(t_path, *MCCanvasPointGet(p_start), *MCCanvasPointGet(p_end));
	if (MCGPathIsValid(t_path))
		MCCanvasPathMakeWithMCGPath(t_path, r_path);
	
	MCGPathRelease(t_path);
}

bool MCCanvasPointsListToMCGPoints(MCProperListRef p_points, MCGPoint *r_points)
{
	bool t_success;
	t_success = true;
	
	MCGPoint *t_points;
	t_points = nil;
	
	uint32_t t_point_count;
	t_point_count = MCProperListGetLength(p_points);
	
	if (t_success)
		t_success = MCMemoryNewArray(t_point_count, t_points);
	
	for (uint32_t i = 0; t_success && i < t_point_count; i++)
	{
		MCValueRef t_value;
		t_value = MCProperListFetchElementAtIndex(p_points, t_point_count);
		
		if (MCValueGetTypeInfo(t_value) == kMCCanvasPointTypeInfo)
		{
			MCCanvasPointGetMCGPoint((MCCanvasPointRef)t_value, t_points[i]);
		}
		else
		{
			// TODO - throw point type error
			t_success = false;
		}
	}
	
	if (t_success)
		r_points = t_points;
	else
		MCMemoryDeleteArray(t_points);
	
	return t_success;
}

void MCCanvasPathMakeWithPoints(bool p_close, MCProperListRef p_points, MCCanvasPathRef &r_path)
{
	bool t_success;
	t_success = true;
	
	MCGPathRef t_path;
	t_path = nil;
	
	if (t_success)
		t_success = MCGPathCreateMutable(t_path);
	
	MCGPoint *t_points;
	t_points = nil;
	
	if (t_success)
		t_success = MCCanvasPointsListToMCGPoints(p_points, t_points);
	
	if (t_success)
	{
		if (p_close)
			MCGPathAddPolygon(t_path, t_points, MCProperListGetLength(p_points));
		else
			MCGPathAddPolyline(t_path, t_points, MCProperListGetLength(p_points));
		
		t_success = MCGPathIsValid(t_path);
	}
	
	if (t_success)
		MCCanvasPathMakeWithMCGPath(t_path, r_path);
	
	MCGPathRelease(t_path);
	MCMemoryDeleteArray(t_points);
}

// Properties

void MCCanvasPathSetMCGPath(MCGPathRef p_path, MCCanvasPathRef &x_path)
{
	MCCanvasPathRef t_path;
	if (!MCCanvasPathCreateWithMCGPath(p_path, t_path))
		return;
	MCValueAssign(x_path, t_path);
	MCValueRelease(t_path);
}

void MCCanvasPathGetSubpaths(integer_t p_start, integer_t p_end, MCCanvasPathRef p_path, MCCanvasPathRef &r_subpaths)
{
	MCGPathRef t_path;
	t_path = nil;
	
	if (!MCGPathMutableCopySubpaths(*MCCanvasPathGet(p_path), p_start, p_end, t_path))
		return;
	
	MCCanvasPathMakeWithMCGPath(t_path, r_subpaths);
	MCGPathRelease(t_path);
}

void MCCanvasPathGetSubpath(integer_t p_index, MCCanvasPathRef p_path, MCCanvasPathRef &r_subpath)
{
	MCCanvasPathGetSubpaths(p_index, p_index, p_path, r_subpath);
}

void MCCanvasPathGetBoundingBox(MCCanvasPathRef p_path, MCCanvasRectangleRef &r_bounds)
{
	MCGRectangle t_rect;
	MCGPathGetBoundingBox(*MCCanvasPathGet(p_path), t_rect);
	
	/* UNCHECKED */ MCCanvasRectangleCreateWithMCGRectangle(t_rect, r_bounds);
}

void MCCanvasPathGetInstructionsAsString(MCCanvasPathRef p_path, MCStringRef &r_instruction_string)
{
	MCStringRef t_instruction_string;
	t_instruction_string = nil;
	if (MCCanvasPathUnparseInstructions(p_path, t_instruction_string))
		r_instruction_string = MCValueRetain(t_instruction_string);
	MCValueRelease(t_instruction_string);
}

// Operations

void MCCanvasPathTransform(MCCanvasPathRef &x_path, const MCGAffineTransform &p_transform)
{
	// Path transformations are applied immediately
	bool t_success;
	t_success = true;
	
	MCGPathRef t_path;
	t_path = nil;

	if (t_success)
	{
		MCGPathMutableCopy(*MCCanvasPathGet(x_path), t_path);
		t_success = MCGPathIsValid(t_path);
	}
	
	if (t_success)
		t_success = MCGPathTransform(t_path, p_transform);
	
	if (t_success)
		MCCanvasPathSetMCGPath(t_path, x_path);
	
	MCGPathRelease(t_path);
}

void MCCanvasPathTransform(MCCanvasPathRef &x_path, MCCanvasTransformRef p_transform)
{
	MCCanvasPathTransform(x_path, *MCCanvasTransformGet(p_transform));
}

void MCCanvasPathScale(MCCanvasPathRef &x_path, MCCanvasFloat p_xscale, MCCanvasFloat p_yscale)
{
	MCCanvasPathTransform(x_path, MCGAffineTransformMakeScale(p_xscale, p_yscale));
}

void MCCanvasPathScaleWithList(MCCanvasPathRef &x_path, MCProperListRef p_scale)
{
	MCGPoint t_scale;
	if (!MCProperListToScale(p_scale, t_scale))
		return;
	
	MCCanvasPathScale(x_path, t_scale.x, t_scale.y);
}

void MCCanvasPathRotate(MCCanvasPathRef &x_path, MCCanvasFloat p_angle)
{
	MCCanvasPathTransform(x_path, MCGAffineTransformMakeRotation(MCCanvasAngleToDegrees(p_angle)));
}

void MCCanvasPathTranslate(MCCanvasPathRef &x_path, MCCanvasFloat p_x, MCCanvasFloat p_y)
{
	MCCanvasPathTransform(x_path, MCGAffineTransformMakeTranslation(p_x, p_y));
}

void MCCanvasPathTranslateWithList(MCCanvasPathRef &x_path, MCProperListRef p_translation)
{
	MCGPoint t_translation;
	if (!MCProperListToTranslation(p_translation, t_translation))
		return;
	
	MCCanvasPathTranslate(x_path, t_translation.x, t_translation.y);
}

void MCCanvasPathAddPath(MCCanvasPathRef p_source, MCCanvasPathRef &x_dest)
{
	bool t_success;
	t_success = true;
	
	MCGPathRef t_path;
	t_path = nil;
	
	if (t_success)
	{
		MCGPathMutableCopy(*MCCanvasPathGet(x_dest), t_path);
		t_success = MCGPathIsValid(t_path);
	}
	
	if (t_success)
	{
		MCGPathAddPath(t_path, *MCCanvasPathGet(p_source));
		t_success = MCGPathIsValid(t_path);
	}
	
	if (t_success)
		MCCanvasPathSetMCGPath(t_path, x_dest);
	
	MCGPathRelease(t_path);
}

void MCCanvasPathMoveTo(MCCanvasPointRef p_point, MCCanvasPathRef &x_path)
{
	bool t_success;
	t_success = true;
	
	MCGPathRef t_path;
	t_path = nil;
	
	if (t_success)
	{
		MCGPathMutableCopy(*MCCanvasPathGet(x_path), t_path);
		t_success = MCGPathIsValid(t_path);
	}
	
	if (t_success)
	{
		MCGPathMoveTo(t_path, *MCCanvasPointGet(p_point));
		t_success = MCGPathIsValid(t_path);
	}
	
	if (t_success)
		MCCanvasPathSetMCGPath(t_path, x_path);
	
	MCGPathRelease(t_path);
}

void MCCanvasPathLineTo(MCCanvasPointRef p_point, MCCanvasPathRef &x_path)
{
	bool t_success;
	t_success = true;
	
	MCGPathRef t_path;
	t_path = nil;
	
	if (t_success)
	{
		MCGPathMutableCopy(*MCCanvasPathGet(x_path), t_path);
		t_success = MCGPathIsValid(t_path);
	}
	
	if (t_success)
	{
		MCGPathLineTo(t_path, *MCCanvasPointGet(p_point));
		t_success = MCGPathIsValid(t_path);
	}
	
	if (t_success)
		MCCanvasPathSetMCGPath(t_path, x_path);
	
	MCGPathRelease(t_path);
}

void MCCanvasPathCurveThroughPoint(MCCanvasPointRef p_through, MCCanvasPointRef p_to, MCCanvasPathRef &x_path)
{
	bool t_success;
	t_success = true;
	
	MCGPathRef t_path;
	t_path = nil;
	
	if (t_success)
	{
		MCGPathMutableCopy(*MCCanvasPathGet(x_path), t_path);
		t_success = MCGPathIsValid(t_path);
	}
	
	if (t_success)
	{
		MCGPathQuadraticTo(t_path, *MCCanvasPointGet(p_through), *MCCanvasPointGet(p_to));
		t_success = MCGPathIsValid(t_path);
	}
	
	if (t_success)
		MCCanvasPathSetMCGPath(t_path, x_path);
	
	MCGPathRelease(t_path);
}

void MCCanvasPathCurveThroughPoints(MCCanvasPointRef p_through_a, MCCanvasPointRef p_through_b, MCCanvasPointRef p_to, MCCanvasPathRef &x_path)
{
	bool t_success;
	t_success = true;
	
	MCGPathRef t_path;
	t_path = nil;
	
	if (t_success)
	{
		MCGPathMutableCopy(*MCCanvasPathGet(x_path), t_path);
		t_success = MCGPathIsValid(t_path);
	}
	
	if (t_success)
	{
		MCGPathCubicTo(t_path, *MCCanvasPointGet(p_through_a), *MCCanvasPointGet(p_through_b), *MCCanvasPointGet(p_to));
		t_success = MCGPathIsValid(t_path);
	}
	
	if (t_success)
		MCCanvasPathSetMCGPath(t_path, x_path);
	
	MCGPathRelease(t_path);
}

void MCCanvasPathClosePath(MCCanvasPathRef &x_path)
{
	bool t_success;
	t_success = true;
	
	MCGPathRef t_path;
	t_path = nil;
	
	if (t_success)
	{
		MCGPathMutableCopy(*MCCanvasPathGet(x_path), t_path);
		t_success = MCGPathIsValid(t_path);
	}
	
	if (t_success)
	{
		MCGPathCloseSubpath(t_path);
		t_success = MCGPathIsValid(t_path);
	}
	
	if (t_success)
		MCCanvasPathSetMCGPath(t_path, x_path);
	
	MCGPathRelease(t_path);
}

//////////

bool MCCanvasPathUnparseInstructionsCallback(void *p_context, MCGPathCommand p_command, MCGPoint *p_points, uint32_t p_point_count)
{
	MCStringRef t_string;
	t_string = static_cast<MCStringRef>(p_context);
	
	MCStringRef t_command_string;
	t_command_string = nil;
	
	bool t_success;
	t_success = true;
	
	if (t_success)
		t_success = MCCanvasPathCommandToString(p_command, t_command_string);
	
	if (t_success)
		t_success = MCStringAppend(t_string, t_command_string);
	
	for (uint32_t i = 0; t_success && i < p_point_count; i++)
		t_success = MCStringAppendFormat(t_string, " %f,%f", p_points[i].x, p_points[i].y);
	
	if (t_success)
		t_success = MCStringAppendChar(t_string, '\n');
	
	return t_success;
}

bool MCCanvasPathUnparseInstructions(MCCanvasPathRef &p_path, MCStringRef &r_string)
{
	MCGPathRef t_path;
	t_path = MCCanvasPathGetMCGPath(p_path);
	
	if (MCGPathIsEmpty(t_path))
		return MCStringCopy(kMCEmptyString, r_string);
	
	bool t_success;
	t_success = true;
	
	MCStringRef t_string;
	t_string = nil;
	
	if (t_success)
		t_success = MCStringCreateMutable(0, t_string);
	
	if (t_success)
		t_success = MCGPathIterate(t_path, MCCanvasPathUnparseInstructionsCallback, t_string);
	
	if (t_success)
		t_success = MCStringCopyAndRelease(t_string, r_string);
	
	if (!t_success)
		MCValueRelease(t_string);
	
	return t_success;
}

bool MCCanvasPathParseInstructions(MCStringRef p_instructions, MCGPathIterateCallback p_callback, void *p_context)
{
	bool t_success;
	t_success = true;
	
	if (MCStringIsEmpty(p_instructions))
	{
		t_success = p_callback(p_context, kMCGPathCommandEnd, nil, 0);
		return t_success;
	}
	
	MCProperListRef t_lines;
	t_lines = nil;

	MCGPathCommand t_command;
	t_command = kMCGPathCommandEnd;
	
	MCGPoint t_points[3];
	uint32_t t_point_count;
	t_point_count = 0;
	
	bool t_ended;
	t_ended = false;

	if (t_success)
		t_success = MCStringSplitByDelimiter(p_instructions, kMCLineEndString, kMCStringOptionCompareExact, t_lines);
	
	for (uint32_t i = 0; t_success && i < MCProperListGetLength(t_lines); i++)
	{
		MCStringRef t_line;
		t_line = (MCStringRef)MCProperListFetchElementAtIndex(t_lines, i);
		
		if (MCStringIsEmpty(t_line))
			continue;
		
		if (t_ended)
			// TODO - throw error - instruction after end
			t_success = false;
		
		MCProperListRef t_components;
		t_components = nil;
		
		if (t_success)
			t_success = MCStringSplitByDelimiter(t_line, MCSTR(" "), kMCStringOptionCompareExact, t_components);
		
		if (t_success)
		{
			MCStringRef t_command_string;
			t_command_string = (MCStringRef)MCProperListFetchElementAtIndex(t_components, 0);
			t_success = MCCanvasPathCommandFromString(t_command_string, t_command);
			// TODO - throw path parse error
		}
		
		if (t_success)
		{
			switch (t_command)
			{
				case kMCGPathCommandMoveTo:
					t_point_count = 1;
					t_success = MCProperListGetLength(t_components) == 2 &&
					MCGPointParse((MCStringRef)MCProperListFetchElementAtIndex(t_components, 1), t_points[0]);
					// TODO - throw parse error
					break;
					
				case kMCGPathCommandLineTo:
					t_point_count = 1;
					t_success = MCProperListGetLength(t_components) == 2 &&
					MCGPointParse((MCStringRef)MCProperListFetchElementAtIndex(t_components, 1), t_points[0]);
					// TODO - throw parse error
					break;
					
				case kMCGPathCommandQuadCurveTo:
					t_point_count = 2;
					t_success = MCProperListGetLength(t_components) == 3 &&
					MCGPointParse((MCStringRef)MCProperListFetchElementAtIndex(t_components, 1), t_points[0]) &&
					MCGPointParse((MCStringRef)MCProperListFetchElementAtIndex(t_components, 2), t_points[1]);
					// TODO - throw parse error
					break;
					
				case kMCGPathCommandCubicCurveTo:
					t_point_count = 3;
					t_success = MCProperListGetLength(t_components) == 4 &&
					MCGPointParse((MCStringRef)MCProperListFetchElementAtIndex(t_components, 1), t_points[0]) &&
					MCGPointParse((MCStringRef)MCProperListFetchElementAtIndex(t_components, 2), t_points[1]) &&
					MCGPointParse((MCStringRef)MCProperListFetchElementAtIndex(t_components, 3), t_points[2]);
					// TODO - throw parse error
					break;
					
				case kMCGPathCommandCloseSubpath:
					t_point_count = 0;
					t_success = MCProperListGetLength(t_components) == 1;
					// TODO - throw parse error
					break;
					
				case kMCGPathCommandEnd:
					t_point_count = 0;
					t_success = MCProperListGetLength(t_components) == 1;
					t_ended = true;
					// TODO - throw parse error
					break;
					
				default:
					MCAssert(false);
			}
		}
		
		MCValueRelease(t_components);
		
		if (t_success)
			t_success = p_callback(p_context, t_command, t_points, t_point_count);
	}
	
	MCValueRelease(t_lines);

	if (t_success && !t_ended)
		t_success = p_callback(p_context, kMCGPathCommandEnd, nil, 0);
	
	return t_success;
}

////////////////////////////////////////////////////////////////////////////////

// Effect

static void __MCCanvasEffectDestroy(MCValueRef p_value)
{
	MCValueRelease(MCCanvasEffectGet((MCCanvasEffectRef)p_value)->color);
}

static bool __MCCanvasEffectCopy(MCValueRef p_value, bool p_release, MCValueRef &r_copy)
{
	if (p_release)
		r_copy = p_value;
	else
		r_copy = MCValueRetain(p_value);
	
	return true;
}

static bool __MCCanvasEffectEqual(MCValueRef p_left, MCValueRef p_right)
{
	if (p_left == p_right)
		return true;
	
	__MCCanvasEffectImpl *t_left, *t_right;
	t_left = MCCanvasEffectGet((MCCanvasEffectRef)p_left);
	t_right = MCCanvasEffectGet((MCCanvasEffectRef)p_right);
	
	if (t_left->type != t_right->type)
		return false;
	
	if (!MCValueIsEqualTo(t_left->color, t_right->color) || t_left->opacity != t_right->opacity || t_left->blend_mode != t_right->blend_mode)
		return false;
	
	switch (t_left->type)
	{
		case kMCCanvasEffectTypeColorOverlay:
			break;
			
		case kMCCanvasEffectTypeInnerGlow:
		case kMCCanvasEffectTypeOuterGlow:
			if (t_left->size != t_right->size || t_left->spread != t_right->spread)
				return false;
			break;
			
		case kMCCanvasEffectTypeInnerShadow:
		case kMCCanvasEffectTypeOuterShadow:
			if (t_left->size != t_right->size || t_left->spread != t_right->spread)
				return false;
			if (t_left->distance != t_right->distance || t_left->angle != t_right->angle)
				return false;
			break;
	}
	
	return true;
}

static hash_t __MCCanvasEffectHash(MCValueRef p_value)
{
	__MCCanvasEffectImpl *t_effect;
	t_effect = MCCanvasEffectGet((MCCanvasEffectRef)p_value);
	
	hash_t t_hash;
	t_hash = MCValueHash(t_effect->color) ^ MCHashInteger(t_effect->blend_mode) ^ MCHashDouble(t_effect->opacity);
	
	switch (t_effect->type)
	{
		case kMCCanvasEffectTypeColorOverlay:
			break;
			
		case kMCCanvasEffectTypeInnerGlow:
		case kMCCanvasEffectTypeOuterGlow:
			t_hash ^= MCHashDouble(t_effect->size) ^ MCHashDouble(t_effect->spread);
			break;
			
		case kMCCanvasEffectTypeInnerShadow:
		case kMCCanvasEffectTypeOuterShadow:
			t_hash ^= MCHashDouble(t_effect->size) ^ MCHashDouble(t_effect->spread);
			t_hash ^= MCHashDouble(t_effect->distance) ^ MCHashDouble(t_effect->angle);
			break;
	}
	
	return t_hash;
}

static bool __MCCanvasEffectDescribe(MCValueRef p_value, MCStringRef &r_desc)
{
	// TODO - implement describe
	return false;
}

bool MCCanvasEffectCreate(const __MCCanvasEffectImpl &p_effect, MCCanvasEffectRef &r_effect)
{
	bool t_success;
	t_success = true;
	
	MCCanvasEffectRef t_effect;
	t_effect = nil;
	
	if (t_success)
		t_success = MCValueCreateCustom(kMCCanvasEffectTypeInfo, sizeof(__MCCanvasEffectImpl), t_effect);
	
	if (t_success)
	{
		*MCCanvasEffectGet(t_effect) = p_effect;
		MCValueRetain(MCCanvasEffectGet(t_effect)->color);
		t_success = MCValueInter(t_effect, r_effect);
	}
	
	MCValueRelease(t_effect);
	
	return t_success;
}

__MCCanvasEffectImpl *MCCanvasEffectGet(MCCanvasEffectRef p_effect)
{
	return (__MCCanvasEffectImpl*)MCValueGetExtraBytesPtr(p_effect);
}

//////////

void MCCanvasEffectEvaluateType(integer_t p_type, integer_t& r_type)
{
	r_type = p_type;
}

// Constructors

void MCCanvasEffectMakeWithPropertyArray(integer_t p_type, MCArrayRef p_properties, MCCanvasEffectRef &r_effect)
{
	// TODO - defaults for missing properties?
	bool t_success;
	t_success = true;
	
	__MCCanvasEffectImpl t_effect;
	t_effect.type = (MCCanvasEffectType)p_type;
	
	real64_t t_opacity;
	
	if (t_success)
		t_success = MCArrayFetchReal(p_properties, s_effect_property_map[kMCCanvasEffectPropertyOpacity], t_opacity);
	
	MCStringRef t_blend_mode;
	t_blend_mode = nil;
	
	// Blend Mode
	if (t_success)
		t_success = MCArrayFetchString(p_properties, s_effect_property_map[kMCCanvasEffectPropertyBlendMode], t_blend_mode) &&
		MCCanvasBlendModeFromString(t_blend_mode, t_effect.blend_mode);
	
	if (t_success)
		t_success = MCArrayFetchCanvasColor(p_properties, s_effect_property_map[kMCCanvasEffectPropertyColor], t_effect.color);
	
	// read properties for each effect type
	switch (t_effect.type)
	{
		case kMCCanvasEffectTypeColorOverlay:
			// no other properties
			break;
			
		case kMCCanvasEffectTypeInnerShadow:
		case kMCCanvasEffectTypeOuterShadow:
		{
			real64_t t_distance, t_angle;
			real64_t t_size, t_spread;
			// distance
			if (t_success)
				t_success = MCArrayFetchReal(p_properties, s_effect_property_map[kMCCanvasEffectPropertyDistance], t_distance);
			// angle
			if (t_success)
				t_success = MCArrayFetchReal(p_properties, s_effect_property_map[kMCCanvasEffectPropertyAngle], t_angle);
			// size
			if (t_success)
				t_success = MCArrayFetchReal(p_properties, s_effect_property_map[kMCCanvasEffectPropertySize], t_size);
			// spread
			if (t_success)
				t_success = MCArrayFetchReal(p_properties, s_effect_property_map[kMCCanvasEffectPropertySpread], t_spread);
			
			t_effect.distance = t_distance;
			t_effect.angle = t_angle;
			t_effect.size = t_size;
			t_effect.spread = t_spread;
			
			break;
		}
			
		case kMCCanvasEffectTypeInnerGlow:
		case kMCCanvasEffectTypeOuterGlow:
		{
			real64_t t_size, t_spread;
			// size
			if (t_success)
				t_success = MCArrayFetchReal(p_properties, s_effect_property_map[kMCCanvasEffectPropertySize], t_size);
			// spread
			if (t_success)
				t_success = MCArrayFetchReal(p_properties, s_effect_property_map[kMCCanvasEffectPropertySpread], t_spread);
			
			t_effect.size = t_size;
			t_effect.spread = t_spread;
			
			break;
		}
			
		default:
			t_success = false;
	}

	// TODO - throw exception on error
	if (t_success)
		t_success = MCCanvasEffectCreate(t_effect, r_effect);
}

//////////

// Properties

void MCCanvasEffectSet(const __MCCanvasEffectImpl &p_effect, MCCanvasEffectRef &x_effect)
{
	MCCanvasEffectRef t_effect;
	t_effect = nil;
	
	if (!MCCanvasEffectCreate(p_effect, t_effect))
		return;
	
	MCValueAssign(x_effect, t_effect);
	MCValueRelease(t_effect);
}

bool MCCanvasEffectHasSizeAndSpread(MCCanvasEffectType p_type)
{
	return p_type == kMCCanvasEffectTypeInnerShadow || p_type == kMCCanvasEffectTypeOuterShadow || p_type == kMCCanvasEffectTypeInnerGlow || p_type == kMCCanvasEffectTypeOuterGlow;
}

bool MCCanvasEffectHasDistanceAndAngle(MCCanvasEffectType p_type)
{
	return p_type == kMCCanvasEffectTypeInnerShadow || p_type == kMCCanvasEffectTypeOuterShadow;
}

void MCCanvasEffectGetTypeAsString(MCCanvasEffectRef p_effect, MCStringRef &r_type)
{
	/* UNCHECKED */ MCCanvasEffectTypeToString(MCCanvasEffectGet(p_effect)->type, r_type);
}

void MCCanvasEffectGetColor(MCCanvasEffectRef p_effect, MCCanvasColorRef &r_color)
{
	r_color = MCValueRetain(MCCanvasEffectGet(p_effect)->color);
}

void MCCanvasEffectSetColor(MCCanvasColorRef p_color, MCCanvasEffectRef &x_effect)
{
	__MCCanvasEffectImpl t_effect;
	t_effect = *MCCanvasEffectGet(x_effect);
	t_effect.color = p_color;
	MCCanvasEffectSet(t_effect, x_effect);
}

void MCCanvasEffectGetBlendModeAsString(MCCanvasEffectRef p_effect, MCStringRef &r_blend_mode)
{
	/* UNCHECKED */ MCCanvasBlendModeToString(MCCanvasEffectGet(p_effect)->blend_mode, r_blend_mode);
}

void MCCanvasEffectSetBlendModeAsString(MCStringRef p_blend_mode, MCCanvasEffectRef &x_effect)
{
	__MCCanvasEffectImpl t_effect;
	t_effect = *MCCanvasEffectGet(x_effect);
	if (!MCCanvasBlendModeFromString(p_blend_mode, t_effect.blend_mode))
	{
		// TODO - throw blend mode error
		return;
	}
	MCCanvasEffectSet(t_effect, x_effect);
}

void MCCanvasEffectGetOpacity(MCCanvasEffectRef p_effect, MCCanvasFloat &r_opacity)
{
	r_opacity = MCCanvasEffectGet(p_effect)->opacity;
}

void MCCanvasEffectSetOpacity(MCCanvasFloat p_opacity, MCCanvasEffectRef &x_effect)
{
	__MCCanvasEffectImpl t_effect;
	t_effect = *MCCanvasEffectGet(x_effect);
	t_effect.opacity = p_opacity;
	MCCanvasEffectSet(t_effect, x_effect);
}

void MCCanvasEffectGetSize(MCCanvasEffectRef p_effect, MCCanvasFloat &r_size)
{
	if (!MCCanvasEffectHasSizeAndSpread(MCCanvasEffectGet(p_effect)->type))
	{
		// TODO - throw error
		return;
	}
	
	r_size = MCCanvasEffectGet(p_effect)->size;
}

void MCCanvasEffectSetSize(MCCanvasFloat p_size, MCCanvasEffectRef &x_effect)
{
	if (!MCCanvasEffectHasSizeAndSpread(MCCanvasEffectGet(x_effect)->type))
	{
		// TODO - throw error
		return;
	}

	__MCCanvasEffectImpl t_effect;
	t_effect = *MCCanvasEffectGet(x_effect);
	t_effect.size = p_size;
	MCCanvasEffectSet(t_effect, x_effect);
}

void MCCanvasEffectGetSpread(MCCanvasEffectRef p_effect, MCCanvasFloat &r_spread)
{
	if (!MCCanvasEffectHasSizeAndSpread(MCCanvasEffectGet(p_effect)->type))
	{
		// TODO - throw error
		return;
	}
	
	r_spread = MCCanvasEffectGet(p_effect)->spread;
}

void MCCanvasEffectSetSpread(MCCanvasFloat p_spread, MCCanvasEffectRef &x_effect)
{
	if (!MCCanvasEffectHasSizeAndSpread(MCCanvasEffectGet(x_effect)->type))
	{
		// TODO - throw error
		return;
	}
	
	__MCCanvasEffectImpl t_effect;
	t_effect = *MCCanvasEffectGet(x_effect);
	t_effect.spread = p_spread;
	MCCanvasEffectSet(t_effect, x_effect);
}

void MCCanvasEffectGetDistance(MCCanvasEffectRef p_effect, MCCanvasFloat &r_distance)
{
	if (!MCCanvasEffectHasDistanceAndAngle(MCCanvasEffectGet(p_effect)->type))
	{
		// TODO - throw error
		return;
	}
	
	r_distance = MCCanvasEffectGet(p_effect)->distance;
}

void MCCanvasEffectSetDistance(MCCanvasFloat p_distance, MCCanvasEffectRef &x_effect)
{
	if (!MCCanvasEffectHasDistanceAndAngle(MCCanvasEffectGet(x_effect)->type))
	{
		// TODO - throw error
		return;
	}
	
	__MCCanvasEffectImpl t_effect;
	t_effect = *MCCanvasEffectGet(x_effect);
	t_effect.distance = p_distance;
	MCCanvasEffectSet(t_effect, x_effect);
}

void MCCanvasEffectGetAngle(MCCanvasEffectRef p_effect, MCCanvasFloat &r_angle)
{
	if (!MCCanvasEffectHasDistanceAndAngle(MCCanvasEffectGet(p_effect)->type))
	{
		// TODO - throw error
		return;
	}
	
	r_angle = MCCanvasEffectGet(p_effect)->angle;
}

void MCCanvasEffectSetAngle(MCCanvasFloat p_angle, MCCanvasEffectRef &x_effect)
{
	if (!MCCanvasEffectHasDistanceAndAngle(MCCanvasEffectGet(x_effect)->type))
	{
		// TODO - throw error
		return;
	}
	
	__MCCanvasEffectImpl t_effect;
	t_effect = *MCCanvasEffectGet(x_effect);
	t_effect.angle = p_angle;
	MCCanvasEffectSet(t_effect, x_effect);
}

////////////////////////////////////////////////////////////////////////////////

// Canvas

void MCCanvasDirtyProperties(__MCCanvasImpl &p_canvas)
{
	p_canvas.antialias_changed = p_canvas.blend_mode_changed = p_canvas.fill_rule_changed = p_canvas.opacity_changed = p_canvas.paint_changed = p_canvas.stippled_changed = true;
}

bool MCCanvasPropertiesInit(MCCanvasProperties &p_properties)
{
	bool t_success;
	t_success = true;
	
	MCCanvasProperties t_properties;
	t_properties.antialias = true;
	t_properties.blend_mode = kMCGBlendModeSourceOver;
	t_properties.fill_rule = kMCGFillRuleEvenOdd;
	t_properties.opacity = 1.0;
	t_properties.paint = nil;
	t_properties.stippled = false;
	t_properties.image_filter = kMCGImageFilterMedium;
	
	MCCanvasSolidPaintRef t_black_paint;
	t_black_paint = nil;
	
	if (t_success)
		t_success = MCCanvasSolidPaintCreateWithColor(kMCCanvasColorBlack, t_black_paint);
	
	if (t_success)
	{
		// TODO - check this cast to supertype?
		t_properties.paint = (MCCanvasPaintRef)t_black_paint;
		
		p_properties = t_properties;
	}
	else
	{
		// TODO - throw error
	}
	
	return t_success;
}

bool MCCanvasPropertiesCopy(MCCanvasProperties &p_src, MCCanvasProperties &p_dst)
{
	MCCanvasProperties t_properties;
	t_properties.antialias = p_src.antialias;
	t_properties.blend_mode = p_src.blend_mode;
	t_properties.fill_rule = p_src.fill_rule;
	t_properties.opacity = p_src.opacity;
	t_properties.stippled = p_src.stippled;
	t_properties.image_filter = p_src.image_filter;
	t_properties.paint = MCValueRetain(p_src.paint);
	
	p_dst = t_properties;
	
	return true;
}

void MCCanvasPropertiesClear(MCCanvasProperties &p_properties)
{
	MCValueRelease(p_properties.paint);
	MCMemoryClear(&p_properties, sizeof(MCCanvasProperties));
}

bool MCCanvasPropertiesPush(__MCCanvasImpl &x_canvas)
{
	if (x_canvas.prop_index <= x_canvas.prop_max)
	{
		if (!MCMemoryResizeArray(x_canvas.prop_max + 1, x_canvas.prop_stack, x_canvas.prop_max))
			return false;
	}
	
	if (!MCCanvasPropertiesCopy(x_canvas.prop_stack[x_canvas.prop_index], x_canvas.prop_stack[x_canvas.prop_index + 1]))
		return false;
	
	x_canvas.prop_index++;
	
	return true;
}

bool MCCanvasPropertiesPop(__MCCanvasImpl &x_canvas)
{
	if (x_canvas.prop_index == 0)
	{
		// TODO - throw canvas pop error
		return false;
	}
	
	MCCanvasPropertiesClear(x_canvas.prop_stack[x_canvas.prop_index]);
	x_canvas.prop_index--;
	
	MCCanvasDirtyProperties(x_canvas);

	return true;
}

bool MCCanvasCreate(MCGContextRef p_context, MCCanvasRef &r_canvas)
{
	bool t_success;
	t_success = true;
	
	MCCanvasRef t_canvas;
	t_canvas = nil;
	
	if (t_success)
		t_success = MCValueCreateCustom(kMCCanvasTypeInfo, sizeof(__MCCanvasImpl), t_canvas);
	
	__MCCanvasImpl *t_canvas_impl;
	t_canvas_impl = nil;

	if (t_success)
	{
		// init property stack with 5 levels
		t_canvas_impl = MCCanvasGet(t_canvas);
		t_success = MCMemoryNewArray(5, t_canvas_impl->prop_stack);
	}
	
	if (t_success)
	{
		t_canvas_impl->prop_max = 5;
		t_success = MCCanvasPropertiesInit(t_canvas_impl->prop_stack[0]);
	}
	
	if (t_success)
	{
		t_canvas_impl->prop_index = 0;
		t_canvas_impl->context = MCGContextRetain(p_context);
		MCCanvasDirtyProperties(*t_canvas_impl);
		
		r_canvas = t_canvas;
	}
	else
		MCValueRelease(t_canvas);
	
	return t_success;
}

__MCCanvasImpl *MCCanvasGet(MCCanvasRef p_canvas)
{
	return (__MCCanvasImpl*)MCValueGetExtraBytesPtr(p_canvas);
}

// MCCanvasRef Type Methods

static void __MCCanvasDestroy(MCValueRef p_canvas)
{
	__MCCanvasImpl *t_canvas;
	t_canvas = (__MCCanvasImpl*)MCValueGetExtraBytesPtr(p_canvas);
	
	if (t_canvas->prop_stack != nil)
	{
		for (uint32_t i = 0; i <= t_canvas->prop_index; i++)
			MCCanvasPropertiesClear(t_canvas->prop_stack[i]);
		MCMemoryDeleteArray(t_canvas->prop_stack);
	}
	
	MCGContextRelease(t_canvas->context);
}

static bool __MCCanvasCopy(MCValueRef p_canvas, bool p_release, MCValueRef &r_copy)
{
	if (p_release)
		r_copy = p_canvas;
	else
		r_copy = MCValueRetain(p_canvas);
	
	return true;
}

static bool __MCCanvasEqual(MCValueRef p_left, MCValueRef p_right)
{
	return p_left == p_right;
}

static hash_t __MCCanvasHash(MCValueRef p_canvas)
{
	return MCHashPointer (p_canvas);
}

static bool __MCCanvasDescribe(MCValueRef p_canvas, MCStringRef &r_description)
{
	// TODO - provide canvas description?
	return false;
}

// Properties

static inline MCCanvasProperties &MCCanvasGetProps(MCCanvasRef p_canvas)
{
	return MCCanvasGet(p_canvas)->props();
}

void MCCanvasGetPaint(MCCanvasRef p_canvas, MCCanvasPaintRef &r_paint)
{
	r_paint = MCValueRetain(MCCanvasGetProps(p_canvas).paint);
}

void MCCanvasSetPaint(MCCanvasPaintRef p_paint, MCCanvasRef p_canvas)
{
	__MCCanvasImpl *t_canvas;
	t_canvas = MCCanvasGet(p_canvas);
	MCValueAssign(t_canvas->props().paint, p_paint);
	t_canvas->paint_changed = true;
}

void MCCanvasGetFillRuleAsString(MCCanvasRef p_canvas, MCStringRef &r_string)
{
	if (!MCCanvasFillRuleToString(MCCanvasGetProps(p_canvas).fill_rule, r_string))
	{
		// TODO - throw error
	}
}

void MCCanvasSetFillRuleAsString(MCStringRef p_string, MCCanvasRef p_canvas)
{
	__MCCanvasImpl *t_canvas;
	t_canvas = MCCanvasGet(p_canvas);
	
	if (!MCCanvasFillRuleFromString(p_string, t_canvas->props().fill_rule))
	{
		// TODO - throw error
		return;
	}
	
	t_canvas->fill_rule_changed = true;
}

void MCCanvasGetAntialias(MCCanvasRef p_canvas, bool &r_antialias)
{
	r_antialias = MCCanvasGetProps(p_canvas).antialias;
}

void MCCanvasSetAntialias(bool p_antialias, MCCanvasRef p_canvas)
{
	__MCCanvasImpl *t_canvas;
	t_canvas = MCCanvasGet(p_canvas);
	
	t_canvas->props().antialias = p_antialias;
	t_canvas->antialias_changed = true;
}

void MCCanvasGetOpacity(MCCanvasRef p_canvas, MCCanvasFloat &r_opacity)
{
	r_opacity = MCCanvasGetProps(p_canvas).opacity;
}

void MCCanvasSetOpacity(MCCanvasFloat p_opacity, MCCanvasRef p_canvas)
{
	__MCCanvasImpl *t_canvas;
	t_canvas = MCCanvasGet(p_canvas);
	
	t_canvas->props().opacity = p_opacity;
	t_canvas->opacity_changed = true;
}

void MCCanvasGetBlendModeAsString(MCCanvasRef p_canvas, MCStringRef &r_blend_mode)
{
	/* UNCHECKED */ MCCanvasBlendModeToString(MCCanvasGetProps(p_canvas).blend_mode, r_blend_mode);
}

void MCCanvasSetBlendModeAsString(MCStringRef p_blend_mode, MCCanvasRef p_canvas)
{
	__MCCanvasImpl *t_canvas;
	t_canvas = MCCanvasGet(p_canvas);
	
	/* UNCHECKED */ MCCanvasBlendModeFromString(p_blend_mode, t_canvas->props().blend_mode);
	t_canvas->blend_mode_changed = true;
}

void MCCanvasGetStippled(MCCanvasRef p_canvas, bool &r_stippled)
{
	r_stippled = MCCanvasGetProps(p_canvas).stippled;
}

void MCCanvasSetStippled(bool p_stippled, MCCanvasRef p_canvas)
{
	__MCCanvasImpl *t_canvas;
	t_canvas = MCCanvasGet(p_canvas);
	
	t_canvas->props().stippled = p_stippled;
	t_canvas->stippled_changed = true;
}

void MCCanvasGetImageResizeQualityAsString(MCCanvasRef p_canvas, MCStringRef &r_quality)
{
	/* UNCHECKED */ MCCanvasImageFilterToString(MCCanvasGetProps(p_canvas).image_filter, r_quality);
}

void MCCanvasSetImageResizeQualityAsString(MCStringRef p_quality, MCCanvasRef p_canvas)
{
	__MCCanvasImpl *t_canvas;
	t_canvas = MCCanvasGet(p_canvas);
	
	/* UNCHECKED */ MCCanvasImageFilterFromString(p_quality, t_canvas->props().image_filter);
	// need to re-apply pattern paint if quality changes
	if (MCCanvasPaintIsPattern(t_canvas->props().paint))
		t_canvas->paint_changed = true;
}

void MCCanvasSetStrokeWidth(MCGFloat p_stroke_width, MCCanvasRef p_canvas)
{
	__MCCanvasImpl *t_canvas;
	t_canvas = MCCanvasGet(p_canvas);
	
	t_canvas->props().stroke_width = p_stroke_width;
	t_canvas->stroke_width_changed = true;
}

void MCCanvasGetStrokeWidth(MCCanvasRef p_canvas, MCGFloat& r_stroke_width)
{
	r_stroke_width = MCCanvasGetProps(p_canvas).stroke_width;
}

//////////

void MCCanvasApplySolidPaint(__MCCanvasImpl &x_canvas, MCCanvasSolidPaintRef p_paint)
{
	__MCCanvasColorImpl *t_color;
	MCCanvasFloat t_red, t_green, t_blue, t_alpha;
	t_color = MCCanvasColorGet(MCCanvasSolidPaintGet(p_paint)->color);
	
	MCGContextSetFillRGBAColor(x_canvas.context, t_color->red, t_color->green, t_color->blue, t_color->alpha);
	MCGContextSetStrokeRGBAColor(x_canvas.context, t_color->red, t_color->green, t_color->blue, t_color->alpha);
}

void MCCanvasApplyPatternPaint(__MCCanvasImpl &x_canvas, MCCanvasPatternRef p_pattern)
{
	__MCCanvasPatternImpl *t_pattern;
	t_pattern = MCCanvasPatternGet(p_pattern);
	
	MCGImageFrame t_frame;
	
	MCGAffineTransform t_pattern_transform;
	t_pattern_transform = *MCCanvasTransformGet(t_pattern->transform);
	
	MCImageRep *t_pattern_image;
	t_pattern_image = MCCanvasImageGetImageRep(t_pattern->image);
	
	MCGAffineTransform t_transform;
	
	MCGAffineTransform t_combined;
	t_combined = MCGAffineTransformConcat(t_pattern_transform, MCGContextGetDeviceTransform(x_canvas.context));
	
	MCGFloat t_scale;
	t_scale = MCGAffineTransformGetEffectiveScale(t_combined);
	
	if (MCImageRepLock(t_pattern_image, 0, t_scale, t_frame))
	{
		t_transform = MCGAffineTransformMakeScale(1.0 / t_frame.x_scale, 1.0 / t_frame.y_scale);
		
		// return image & transform scaled for image density
		t_transform = MCGAffineTransformConcat(t_pattern_transform, t_transform);
		

		MCGContextSetFillPattern(x_canvas.context, t_frame.image, t_transform, x_canvas.props().image_filter);
		MCGContextSetStrokePattern(x_canvas.context, t_frame.image, t_transform, x_canvas.props().image_filter);
		
		MCImageRepUnlock(t_pattern_image, 0, t_frame);
	}
	
}

bool MCCanvasApplyGradientPaint(__MCCanvasImpl &x_canvas, MCCanvasGradientRef p_gradient)
{
	bool t_success;
	t_success = true;
	
	__MCCanvasGradientImpl *t_gradient;
	t_gradient = MCCanvasGradientGet(p_gradient);
	
	MCGFloat *t_offsets;
	t_offsets = nil;
	
	MCGColor *t_colors;
	t_colors = nil;
	
	MCGAffineTransform t_gradient_transform;
	t_gradient_transform = *MCCanvasTransformGet(t_gradient->transform);
	
	uint32_t t_ramp_length;
	t_ramp_length = MCProperListGetLength(t_gradient->ramp);
	
	if (t_success)
		t_success = MCMemoryNewArray(t_ramp_length, t_offsets) && MCMemoryNewArray(t_ramp_length, t_colors);
	
	if (t_success)
	{
		for (uint32_t i = 0; i < t_ramp_length; i++)
		{
			MCCanvasGradientStopRef t_stop;
			/* UNCHECKED */ MCProperListFetchGradientStopAt(t_gradient->ramp, i, t_stop);
			t_offsets[i] = MCCanvasGradientStopGet(t_stop)->offset;
			t_colors[i] = MCCanvasColorToMCGColor(MCCanvasGradientStopGet(t_stop)->color);
		}
		
		MCGContextSetFillGradient(x_canvas.context, t_gradient->function, t_offsets, t_colors, t_ramp_length, t_gradient->mirror, t_gradient->wrap, t_gradient->repeats, t_gradient_transform, t_gradient->filter);
		MCGContextSetStrokeGradient(x_canvas.context, t_gradient->function, t_offsets, t_colors, t_ramp_length, t_gradient->mirror, t_gradient->wrap, t_gradient->repeats, t_gradient_transform, t_gradient->filter);
	}

	MCMemoryDeleteArray(t_offsets);
	MCMemoryDeleteArray(t_colors);
	
	return t_success;
}

void MCCanvasApplyPaint(__MCCanvasImpl &x_canvas, MCCanvasPaintRef &p_paint)
{
	if (MCCanvasPaintIsSolidPaint(p_paint))
		MCCanvasApplySolidPaint(x_canvas, (MCCanvasSolidPaintRef)p_paint);
	else if (MCCanvasPaintIsPattern(p_paint))
		MCCanvasApplyPatternPaint(x_canvas, (MCCanvasPatternRef)p_paint);
	else if (MCCanvasPaintIsGradient(p_paint))
		MCCanvasApplyGradientPaint(x_canvas, (MCCanvasGradientRef)p_paint);
	else
		MCAssert(false);
}

void MCCanvasApplyChanges(__MCCanvasImpl &x_canvas)
{
	if (x_canvas.paint_changed)
	{
		MCCanvasApplyPaint(x_canvas, x_canvas.props().paint);
		x_canvas.paint_changed = false;
	}
	
	if (x_canvas.fill_rule_changed)
	{
		MCGContextSetFillRule(x_canvas.context, x_canvas.props().fill_rule);
		x_canvas.fill_rule_changed = false;
	}
	
	if (x_canvas.antialias_changed)
	{
		MCGContextSetShouldAntialias(x_canvas.context, x_canvas.props().antialias);
		x_canvas.antialias_changed = false;
	}
	
	if (x_canvas.opacity_changed)
	{
		MCGContextSetOpacity(x_canvas.context, x_canvas.props().opacity);
		x_canvas.opacity_changed = false;
	}
	
	if (x_canvas.blend_mode_changed)
	{
		MCGContextSetBlendMode(x_canvas.context, x_canvas.props().blend_mode);
		x_canvas.blend_mode_changed = false;
	}
	
	if (x_canvas.stippled_changed)
	{
		MCGPaintStyle t_style;
		t_style = x_canvas.props().stippled ? kMCGPaintStyleStippled : kMCGPaintStyleOpaque;
		MCGContextSetFillPaintStyle(x_canvas.context, t_style);
		MCGContextSetStrokePaintStyle(x_canvas.context, t_style);
		x_canvas.stippled_changed = false;
	}
    
    if (x_canvas . stroke_width_changed)
    {
        MCGContextSetStrokeWidth(x_canvas.context, x_canvas.props().stroke_width);
        x_canvas.stroke_width_changed = false;
    }
}

//////////

void MCCanvasTransform(MCCanvasRef p_canvas, const MCGAffineTransform &p_transform)
{
	__MCCanvasImpl *t_canvas;
	t_canvas = MCCanvasGet(p_canvas);
	
	MCGContextConcatCTM(t_canvas->context, p_transform);
	// Need to re-apply pattern paint when transform changes
	if (MCCanvasPaintIsPattern(t_canvas->props().paint))
		t_canvas->paint_changed = true;
}

void MCCanvasTransform(MCCanvasRef p_canvas, MCCanvasTransformRef p_transform)
{
	MCCanvasTransform(p_canvas, *MCCanvasTransformGet(p_transform));
}

void MCCanvasScale(MCCanvasRef p_canvas, MCCanvasFloat p_scale_x, MCCanvasFloat p_scale_y)
{
	MCCanvasTransform(p_canvas, MCGAffineTransformMakeScale(p_scale_x, p_scale_y));
}

void MCCanvasScaleWithList(MCCanvasRef p_canvas, MCProperListRef p_scale)
{
	MCGPoint t_scale;
	if (!MCProperListToScale(p_scale, t_scale))
		return;
	
	MCCanvasScale(p_canvas, t_scale.x, t_scale.y);
}

void MCCanvasRotate(MCCanvasRef p_canvas, MCCanvasFloat p_angle)
{
	MCCanvasTransform(p_canvas, MCGAffineTransformMakeRotation(MCCanvasAngleToDegrees(p_angle)));
}

void MCCanvasTranslate(MCCanvasRef p_canvas, MCCanvasFloat p_x, MCCanvasFloat p_y)
{
	MCCanvasTransform(p_canvas, MCGAffineTransformMakeTranslation(p_x, p_y));
}

void MCCanvasTranslateWithList(MCCanvasRef p_canvas, MCProperListRef p_translation)
{
	MCGPoint t_translation;
	if (!MCProperListToTranslation(p_translation, t_translation))
		return;
	
	MCCanvasTranslate(p_canvas, t_translation.x, t_translation.y);
}

//////////

void MCCanvasSaveState(MCCanvasRef p_canvas)
{
	__MCCanvasImpl *t_canvas;
	t_canvas = MCCanvasGet(p_canvas);
	
	if (!MCCanvasPropertiesPush(*t_canvas))
		return;

	MCGContextSave(t_canvas->context);
}

void MCCanvasRestoreState(MCCanvasRef p_canvas)
{
	__MCCanvasImpl *t_canvas;
	t_canvas = MCCanvasGet(p_canvas);
	
	if (!MCCanvasPropertiesPop(*t_canvas))
		return;

	MCGContextRestore(t_canvas->context);
}

void MCCanvasBeginLayer(MCCanvasRef p_canvas)
{
	__MCCanvasImpl *t_canvas;
	t_canvas = MCCanvasGet(p_canvas);
	
	MCCanvasApplyChanges(*t_canvas);
	if (!MCCanvasPropertiesPush(*t_canvas))
		return;

	MCGContextBegin(t_canvas->context, true);
}

static void MCPolarCoordsToCartesian(MCGFloat p_distance, MCGFloat p_angle, MCGFloat &r_x, MCGFloat &r_y)
{
	r_x = p_distance * cos(p_angle);
	r_y = p_distance * sin(p_angle);
}


void MCCanvasBeginLayerWithEffect(MCCanvasEffectRef p_effect, MCCanvasRef p_canvas)
{
	__MCCanvasImpl *t_canvas;
	t_canvas = MCCanvasGet(p_canvas);
	
	MCCanvasApplyChanges(*t_canvas);
	if (!MCCanvasPropertiesPush(*t_canvas))
		return;

	MCGBitmapEffects t_effects;
	t_effects.has_color_overlay = t_effects.has_drop_shadow = t_effects.has_inner_glow = t_effects.has_inner_shadow = t_effects.has_outer_glow = false;
	
	__MCCanvasEffectImpl *t_effect_impl;
	t_effect_impl = MCCanvasEffectGet(p_effect);
	
	switch (t_effect_impl->type)
	{
		case kMCCanvasEffectTypeColorOverlay:
		{
			t_effects.has_color_overlay = true;
			t_effects.color_overlay.blend_mode = t_effect_impl->blend_mode;
			t_effects.color_overlay.color = MCCanvasColorToMCGColor(t_effect_impl->color);
			break;
		}
			
		case kMCCanvasEffectTypeInnerGlow:
		{
			t_effects.has_inner_glow = true;
			t_effects.inner_glow.blend_mode = t_effect_impl->blend_mode;
			t_effects.inner_glow.color = MCCanvasColorToMCGColor(t_effect_impl->color);
//			t_effects.inner_glow.inverted = // TODO - inverted property?
			t_effects.inner_glow.size = t_effect_impl->size;
			t_effects.inner_glow.spread = t_effect_impl->spread;
			break;
		}
			
		case kMCCanvasEffectTypeInnerShadow:
		{
			t_effects.has_inner_shadow = true;
			t_effects.inner_shadow.blend_mode = t_effect_impl->blend_mode;
			t_effects.inner_shadow.color = MCCanvasColorToMCGColor(t_effect_impl->color);
//			t_effects.inner_shadow.knockout = // TODO - knockout property?
			t_effects.inner_shadow.size = t_effect_impl->size;
			t_effects.inner_shadow.spread = t_effect_impl->spread;
			MCPolarCoordsToCartesian(t_effect_impl->distance, MCCanvasAngleToRadians(t_effect_impl->angle), t_effects.inner_shadow.x_offset, t_effects.inner_shadow.y_offset);
			break;
		}
			
		case kMCCanvasEffectTypeOuterGlow:
		{
			t_effects.has_outer_glow = true;
			t_effects.outer_glow.blend_mode = t_effect_impl->blend_mode;
			t_effects.outer_glow.color = MCCanvasColorToMCGColor(t_effect_impl->color);
			t_effects.outer_glow.size = t_effect_impl->size;
			t_effects.outer_glow.spread = t_effect_impl->spread;
			break;
		}
			
		case kMCCanvasEffectTypeOuterShadow:
		{
			t_effects.has_drop_shadow = true;
			t_effects.drop_shadow.blend_mode = t_effect_impl->blend_mode;
			t_effects.drop_shadow.color = MCCanvasColorToMCGColor(t_effect_impl->color);
			t_effects.drop_shadow.size = t_effect_impl->size;
			t_effects.drop_shadow.spread = t_effect_impl->spread;
			MCPolarCoordsToCartesian(t_effect_impl->distance, MCCanvasAngleToRadians(t_effect_impl->angle), t_effects.drop_shadow.x_offset, t_effects.drop_shadow.y_offset);
			break;
		}
			
		default:
			MCAssert(false);
	}
	
	MCGRectangle t_rect;
	t_rect = MCGContextGetClipBounds(t_canvas->context);
	MCGContextBeginWithEffects(t_canvas->context, t_rect, t_effects);
}

void MCCanvasEndLayer(MCCanvasRef p_canvas)
{
	__MCCanvasImpl *t_canvas;
	t_canvas = MCCanvasGet(p_canvas);
	
	if (!MCCanvasPropertiesPop(*t_canvas))
		return;

	MCGContextEnd(t_canvas->context);
}

void MCCanvasFill(MCCanvasRef p_canvas)
{
	__MCCanvasImpl *t_canvas;
	t_canvas = MCCanvasGet(p_canvas);
	
	MCCanvasApplyChanges(*t_canvas);
	MCGContextFill(t_canvas->context);
}

void MCCanvasStroke(MCCanvasRef p_canvas)
{
	__MCCanvasImpl *t_canvas;
	t_canvas = MCCanvasGet(p_canvas);
	
	MCCanvasApplyChanges(*t_canvas);
	MCGContextStroke(t_canvas->context);
}

void MCCanvasClipToRect(MCCanvasRectangleRef &p_rect, MCCanvasRef p_canvas)
{
	__MCCanvasImpl *t_canvas;
	t_canvas = MCCanvasGet(p_canvas);
	
	MCGContextClipToRect(t_canvas->context, *MCCanvasRectangleGet(p_rect));
}

void MCCanvasAddPath(MCCanvasPathRef p_path, MCCanvasRef p_canvas)
{
	__MCCanvasImpl *t_canvas;
	t_canvas = MCCanvasGet(p_canvas);
	
	MCGContextAddPath(t_canvas->context, *MCCanvasPathGet(p_path));
}

void MCCanvasFillPath(MCCanvasPathRef p_path, MCCanvasRef p_canvas)
{
	MCCanvasAddPath(p_path, p_canvas);
	MCCanvasFill(p_canvas);
}

void MCCanvasStrokePath(MCCanvasPathRef p_path, MCCanvasRef p_canvas)
{
	MCCanvasAddPath(p_path, p_canvas);
	MCCanvasStroke(p_canvas);
}

void MCCanvasDrawRectOfImage(MCCanvasRef p_canvas, MCCanvasImageRef p_image, const MCGRectangle &p_src_rect, const MCGRectangle &p_dst_rect)
{
	__MCCanvasImpl *t_canvas;
	t_canvas = MCCanvasGet(p_canvas);
	
	MCImageRep *t_image;
	t_image = MCCanvasImageGetImageRep(p_image);
	
	MCCanvasApplyChanges(*t_canvas);

	MCGImageFrame t_frame;
	
	MCGFloat t_scale;
	t_scale = MCGAffineTransformGetEffectiveScale(MCGContextGetDeviceTransform(t_canvas->context));
	
	if (MCImageRepLock(t_image, 0, t_scale, t_frame))
	{
		MCGAffineTransform t_transform;
		t_transform = MCGAffineTransformMakeScale(1.0 / t_frame.x_scale, 1.0 / t_frame.y_scale);
		
		MCGRectangle t_src_rect;
		t_src_rect = MCGRectangleScale(p_src_rect, t_frame.x_scale, t_frame.y_scale);
		
		MCGContextDrawRectOfImage(t_canvas->context, t_frame.image, t_src_rect, p_dst_rect, t_canvas->props().image_filter);
		
		MCImageRepUnlock(t_image, 0, t_frame);
	}
}

void MCCanvasDrawRectOfImage(MCCanvasRectangleRef p_src_rect, MCCanvasImageRef p_image, MCCanvasRectangleRef p_dst_rect, MCCanvasRef p_canvas)
{
	MCCanvasDrawRectOfImage(p_canvas, p_image, *MCCanvasRectangleGet(p_src_rect), *MCCanvasRectangleGet(p_dst_rect));
}

void MCCanvasDrawImage(MCCanvasImageRef p_image, MCCanvasRectangleRef p_dst_rect, MCCanvasRef p_canvas)
{
	MCGRectangle t_src_rect;
	uint32_t t_width, t_height;
	MCCanvasImageGetWidth(p_image, t_width);
	MCCanvasImageGetHeight(p_image, t_height);
	t_src_rect = MCGRectangleMake(0, 0, t_width, t_height);
	MCCanvasDrawRectOfImage(p_canvas, p_image, t_src_rect, *MCCanvasRectangleGet(p_dst_rect));
}

void MCCanvasMoveTo(MCCanvasPointRef p_point, MCCanvasRef p_canvas)
{
	__MCCanvasImpl *t_canvas;
	t_canvas = MCCanvasGet(p_canvas);
	
	MCGContextMoveTo(t_canvas->context, *MCCanvasPointGet(p_point));
}

void MCCanvasLineTo(MCCanvasPointRef p_point, MCCanvasRef p_canvas)
{
	__MCCanvasImpl *t_canvas;
	t_canvas = MCCanvasGet(p_canvas);
	
	MCGContextLineTo(t_canvas->context, *MCCanvasPointGet(p_point));
}

void MCCanvasCurveThroughPoint(MCCanvasPointRef p_through, MCCanvasPointRef p_to, MCCanvasRef p_canvas)
{
	__MCCanvasImpl *t_canvas;
	t_canvas = MCCanvasGet(p_canvas);
	
	MCGContextQuadraticTo(t_canvas->context, *MCCanvasPointGet(p_through), *MCCanvasPointGet(p_to));
}

void MCCanvasCurveThroughPoints(MCCanvasPointRef p_through_a, MCCanvasPointRef p_through_b, MCCanvasPointRef p_to, MCCanvasRef p_canvas)
{
	__MCCanvasImpl *t_canvas;
	t_canvas = MCCanvasGet(p_canvas);
	
	MCGContextCubicTo(t_canvas->context, *MCCanvasPointGet(p_through_a), *MCCanvasPointGet(p_through_b), *MCCanvasPointGet(p_to));
}

void MCCanvasClosePath(MCCanvasRef p_canvas)
{
	__MCCanvasImpl *t_canvas;
	t_canvas = MCCanvasGet(p_canvas);
	
	MCGContextCloseSubpath(t_canvas->context);
}

////////////////////////////////////////////////////////////////////////////////

static MCCanvasRef s_current_canvas = nil;

void MCCanvasPush(MCGContextRef p_gcontext, uintptr_t& r_cookie)
{
    MCCanvasRef t_new_canvas;
    MCCanvasCreate(p_gcontext, t_new_canvas);
    r_cookie = (uintptr_t)s_current_canvas;
    s_current_canvas = t_new_canvas;
}

void MCCanvasPop(uintptr_t p_cookie)
{
    MCCanvasRef t_canvas;
    t_canvas = s_current_canvas;
    s_current_canvas = (MCCanvasRef)p_cookie;
    MCValueRelease(t_canvas);
}

extern "C" MC_DLLEXPORT void MCCanvasThisCanvas(MCCanvasRef& r_canvas)
{
    r_canvas = MCValueRetain(s_current_canvas);
}

extern "C" MC_DLLEXPORT void MCCanvasPretendToAssignToThisCanvas(MCCanvasRef p_canvas)
{
}

////////////////////////////////////////////////////////////////////////////////

static MCValueCustomCallbacks kMCCanvasRectangleCustomValueCallbacks =
{
	false,
	__MCCanvasRectangleDestroy,
	__MCCanvasRectangleCopy,
	__MCCanvasRectangleEqual,
	__MCCanvasRectangleHash,
	__MCCanvasRectangleDescribe,
};

static MCValueCustomCallbacks kMCCanvasPointCustomValueCallbacks =
{
	false,
	__MCCanvasPointDestroy,
	__MCCanvasPointCopy,
	__MCCanvasPointEqual,
	__MCCanvasPointHash,
	__MCCanvasPointDescribe,
};

static MCValueCustomCallbacks kMCCanvasColorCustomValueCallbacks =
{
	false,
	__MCCanvasColorDestroy,
	__MCCanvasColorCopy,
	__MCCanvasColorEqual,
	__MCCanvasColorHash,
	__MCCanvasColorDescribe,
};

static MCValueCustomCallbacks kMCCanvasTransformCustomValueCallbacks =
{
	false,
	__MCCanvasTransformDestroy,
	__MCCanvasTransformCopy,
	__MCCanvasTransformEqual,
	__MCCanvasTransformHash,
	__MCCanvasTransformDescribe,
};

static MCValueCustomCallbacks kMCCanvasImageCustomValueCallbacks =
{
	false,
	__MCCanvasImageDestroy,
	__MCCanvasImageCopy,
	__MCCanvasImageEqual,
	__MCCanvasImageHash,
	__MCCanvasImageDescribe,
};

static MCValueCustomCallbacks kMCCanvasPaintCustomValueCallbacks =
{
	false,
	nil,
	nil,
	nil,
	nil,
	nil,
};

static MCValueCustomCallbacks kMCCanvasSolidPaintCustomValueCallbacks =
{
	false,
	__MCCanvasSolidPaintDestroy,
	__MCCanvasSolidPaintCopy,
	__MCCanvasSolidPaintEqual,
	__MCCanvasSolidPaintHash,
	__MCCanvasSolidPaintDescribe,
};

static MCValueCustomCallbacks kMCCanvasPatternCustomValueCallbacks =
{
	false,
	__MCCanvasPatternDestroy,
	__MCCanvasPatternCopy,
	__MCCanvasPatternEqual,
	__MCCanvasPatternHash,
	__MCCanvasPatternDescribe,
};

static MCValueCustomCallbacks kMCCanvasGradientCustomValueCallbacks =
{
	false,
	__MCCanvasGradientDestroy,
	__MCCanvasGradientCopy,
	__MCCanvasGradientEqual,
	__MCCanvasGradientHash,
	__MCCanvasGradientDescribe,
};

static MCValueCustomCallbacks kMCCanvasGradientStopCustomValueCallbacks =
{
	false,
	__MCCanvasGradientStopDestroy,
	__MCCanvasGradientStopCopy,
	__MCCanvasGradientStopEqual,
	__MCCanvasGradientStopHash,
	__MCCanvasGradientStopDescribe,
};

static MCValueCustomCallbacks kMCCanvasPathCustomValueCallbacks =
{
	false,
	__MCCanvasPathDestroy,
	__MCCanvasPathCopy,
	__MCCanvasPathEqual,
	__MCCanvasPathHash,
	__MCCanvasPathDescribe,
};

static MCValueCustomCallbacks kMCCanvasEffectCustomValueCallbacks =
{
	false,
	__MCCanvasEffectDestroy,
	__MCCanvasEffectCopy,
	__MCCanvasEffectEqual,
	__MCCanvasEffectHash,
	__MCCanvasEffectDescribe,
};

static MCValueCustomCallbacks kMCCanvasCustomValueCallbacks =
{
	true,
	__MCCanvasDestroy,
	__MCCanvasCopy,
	__MCCanvasEqual,
	__MCCanvasHash,
	__MCCanvasDescribe,
};

static void __create_named_custom_typeinfo(MCTypeInfoRef p_base, const MCValueCustomCallbacks *p_callbacks, MCNameRef p_name, MCTypeInfoRef& r_typeinfo)
{
    MCAutoTypeInfoRef t_unnamed;
    /* UNCHECKED */ MCCustomTypeInfoCreate(p_base, p_callbacks, &t_unnamed);
    /* UNCHECKED */ MCNamedTypeInfoCreate(p_name, r_typeinfo);
    /* UNCHECKED */ MCNamedTypeInfoBind(r_typeinfo, *t_unnamed);
}

void MCCanvasTypesInitialize()
{
	/* UNCHECKED */ __create_named_custom_typeinfo(kMCNullTypeInfo, &kMCCanvasRectangleCustomValueCallbacks, MCNAME("com.livecode.canvas.Rectangle"), kMCCanvasRectangleTypeInfo);
	/* UNCHECKED */ __create_named_custom_typeinfo(kMCNullTypeInfo, &kMCCanvasPointCustomValueCallbacks, MCNAME("com.livecode.canvas.Point"), kMCCanvasPointTypeInfo);
	/* UNCHECKED */ __create_named_custom_typeinfo(kMCNullTypeInfo, &kMCCanvasColorCustomValueCallbacks, MCNAME("com.livecode.canvas.Color"), kMCCanvasColorTypeInfo);
	/* UNCHECKED */ __create_named_custom_typeinfo(kMCNullTypeInfo, &kMCCanvasTransformCustomValueCallbacks, MCNAME("com.livecode.canvas.Transform"), kMCCanvasTransformTypeInfo);
	/* UNCHECKED */ __create_named_custom_typeinfo(kMCNullTypeInfo, &kMCCanvasImageCustomValueCallbacks, MCNAME("com.livecode.canvas.Image"), kMCCanvasImageTypeInfo);
	/* UNCHECKED */ __create_named_custom_typeinfo(kMCNullTypeInfo, &kMCCanvasPaintCustomValueCallbacks, MCNAME("com.livecode.canvas.Paint"), kMCCanvasPaintTypeInfo);
	/* UNCHECKED */ __create_named_custom_typeinfo(kMCCanvasPaintTypeInfo, &kMCCanvasSolidPaintCustomValueCallbacks, MCNAME("com.livecode.canvas.SolidPaint"), kMCCanvasSolidPaintTypeInfo);
	/* UNCHECKED */ __create_named_custom_typeinfo(kMCCanvasPaintTypeInfo, &kMCCanvasPatternCustomValueCallbacks, MCNAME("com.livecode.canvas.Pattern"), kMCCanvasPatternTypeInfo);
	/* UNCHECKED */ __create_named_custom_typeinfo(kMCCanvasPaintTypeInfo, &kMCCanvasGradientCustomValueCallbacks, MCNAME("com.livecode.canvas.Gradient"), kMCCanvasGradientTypeInfo);
	/* UNCHECKED */ __create_named_custom_typeinfo(kMCNullTypeInfo, &kMCCanvasGradientStopCustomValueCallbacks, MCNAME("com.livecode.canvas.GradientStop"), kMCCanvasGradientStopTypeInfo);
	/* UNCHECKED */ __create_named_custom_typeinfo(kMCNullTypeInfo, &kMCCanvasPathCustomValueCallbacks, MCNAME("com.livecode.canvas.Path"), kMCCanvasPathTypeInfo);
	/* UNCHECKED */ __create_named_custom_typeinfo(kMCNullTypeInfo, &kMCCanvasEffectCustomValueCallbacks, MCNAME("com.livecode.canvas.Effect"), kMCCanvasEffectTypeInfo);
	/* UNCHECKED */ __create_named_custom_typeinfo(kMCNullTypeInfo, &kMCCanvasCustomValueCallbacks, MCNAME("com.livecode.canvas.Canvas"), kMCCanvasTypeInfo);
}

void MCCanvasTypesFinalize()
{
	MCValueRelease(kMCCanvasRectangleTypeInfo);
	MCValueRelease(kMCCanvasPointTypeInfo);
	MCValueRelease(kMCCanvasColorTypeInfo);
	MCValueRelease(kMCCanvasTransformTypeInfo);
	MCValueRelease(kMCCanvasImageTypeInfo);
	MCValueRelease(kMCCanvasPaintTypeInfo);
	MCValueRelease(kMCCanvasGradientStopTypeInfo);
	MCValueRelease(kMCCanvasPathTypeInfo);
	MCValueRelease(kMCCanvasEffectTypeInfo);
	MCValueRelease(kMCCanvasTypeInfo);
}

////////////////////////////////////////////////////////////////////////////////

bool MCCanvasCreateNamedErrorType(MCNameRef p_name, MCStringRef p_message, MCTypeInfoRef &r_error_type)
{
	MCAutoTypeInfoRef t_type, t_named_type;
	
	if (!MCErrorTypeInfoCreate(MCNAME("canvas"), p_message, &t_type))
		return false;
	
	if (!MCNamedTypeInfoCreate(p_name, &t_named_type))
		return false;
	
	if (!MCNamedTypeInfoBind(*t_named_type, *t_type))
		return false;
	
	r_error_type = MCValueRetain(*t_named_type);
	return true;
}

bool MCCanvasThrowError(MCTypeInfoRef p_error_type)
{
	MCAutoValueRefBase<MCErrorRef> t_error;
	if (!MCErrorCreate(p_error_type, nil, &t_error))
		return false;
	
	return MCErrorThrow(*t_error);
}

void MCCanvasErrorsInitialize()
{
	kMCCanvasRectangleListFormatErrorTypeInfo = nil;
	/* UNCHECKED */ MCCanvasCreateNamedErrorType(MCNAME("com.livecode.canvas.RectangleListFormatError"), MCSTR("Rectangle parameter must be a list of 4 numbers."), kMCCanvasRectangleListFormatErrorTypeInfo);
	
	kMCCanvasPointListFormatErrorTypeInfo = nil;
	/* UNCHECKED */ MCCanvasCreateNamedErrorType(MCNAME("com.livecode.canvas.PointListFormatError"), MCSTR("Point parameter must be a list of 2 numbers."), kMCCanvasPointListFormatErrorTypeInfo);
	
	kMCCanvasColorListFormatErrorTypeInfo = nil;
	/* UNCHECKED */ MCCanvasCreateNamedErrorType(MCNAME("com.livecode.canvas.ColorListFormatError"), MCSTR("Color parameter must be a list of 3 or 4 numbers between 0 and 1."), kMCCanvasColorListFormatErrorTypeInfo);
	
	kMCCanvasScaleListFormatErrorTypeInfo = nil;
	/* UNCHECKED */ MCCanvasCreateNamedErrorType(MCNAME("com.livecode.canvas.ScaleListFormatError"), MCSTR("Scale parameter must be a list of 1 or 2 numbers."), kMCCanvasScaleListFormatErrorTypeInfo);

	kMCCanvasTranslationListFormatErrorTypeInfo = nil;
	/* UNCHECKED */ MCCanvasCreateNamedErrorType(MCNAME("com.livecode.canvas.TranslationListFormatError"), MCSTR("Translation parameter must be a list of 2 numbers."), kMCCanvasTranslationListFormatErrorTypeInfo);

	kMCCanvasSkewListFormatErrorTypeInfo = nil;
	/* UNCHECKED */ MCCanvasCreateNamedErrorType(MCNAME("com.livecode.canvas.SkewListFormatError"), MCSTR("Skew parameter must be a list of 2 numbers."), kMCCanvasSkewListFormatErrorTypeInfo);

	kMCCanvasRadiiListFormatErrorTypeInfo = nil;
	/* UNCHECKED */ MCCanvasCreateNamedErrorType(MCNAME("com.livecode.canvas.RadiiListFormatError"), MCSTR("Radii parameter must be a list of 2 numbers."), kMCCanvasRadiiListFormatErrorTypeInfo);
	
	kMCCanvasImageSizeListFormatErrorTypeInfo = nil;
	/* UNCHECKED */ MCCanvasCreateNamedErrorType(MCNAME("com.livecode.canvas.ImageSizeListFormatError"), MCSTR("image size parameter must be a list of 2 integers greater than 0."), kMCCanvasImageSizeListFormatErrorTypeInfo);
	
}

void MCCanvasErrorsFinalize()
{
	MCValueRelease(kMCCanvasRectangleListFormatErrorTypeInfo);
	MCValueRelease(kMCCanvasPointListFormatErrorTypeInfo);
	MCValueRelease(kMCCanvasColorListFormatErrorTypeInfo);
	MCValueRelease(kMCCanvasScaleListFormatErrorTypeInfo);
	MCValueRelease(kMCCanvasTranslationListFormatErrorTypeInfo);
	MCValueRelease(kMCCanvasSkewListFormatErrorTypeInfo);
	MCValueRelease(kMCCanvasRadiiListFormatErrorTypeInfo);
	MCValueRelease(kMCCanvasImageSizeListFormatErrorTypeInfo);
}

////////////////////////////////////////////////////////////////////////////////

void MCCanvasConstantsInitialize()
{
	/* UNCHECKED */ MCCanvasTransformCreateWithMCGAffineTransform(MCGAffineTransformMakeIdentity(), kMCCanvasIdentityTransform);
	/* UNCHECKED */ MCCanvasColorCreateWithRGBA(0, 0, 0, 1, kMCCanvasColorBlack);
}

void MCCanvasConstantsFinalize()
{
	MCValueRelease(kMCCanvasIdentityTransform);
	MCValueRelease(kMCCanvasColorBlack);
}

////////////////////////////////////////////////////////////////////////////////

void MCCanvasStringsInitialize()
{
	MCMemoryClear(s_blend_mode_map, sizeof(s_blend_mode_map));
	MCMemoryClear(s_transform_matrix_keys, sizeof(s_transform_matrix_keys));
	MCMemoryClear(s_effect_type_map, sizeof(s_effect_type_map));
	MCMemoryClear(s_effect_property_map, sizeof(s_effect_property_map));
	MCMemoryClear(s_gradient_type_map, sizeof(s_gradient_type_map));
	MCMemoryClear(s_canvas_fillrule_map, sizeof(s_gradient_type_map));
	MCMemoryClear(s_image_filter_map, sizeof(s_image_filter_map));
	
	/* UNCHECKED */
	s_blend_mode_map[kMCGBlendModeClear] = MCNAME("clear");
	s_blend_mode_map[kMCGBlendModeCopy] = MCNAME("copy");
	s_blend_mode_map[kMCGBlendModeSourceOut] = MCNAME("source over");
	s_blend_mode_map[kMCGBlendModeSourceIn] = MCNAME("source in");
	s_blend_mode_map[kMCGBlendModeSourceOut] = MCNAME("source out");
	s_blend_mode_map[kMCGBlendModeSourceAtop] = MCNAME("source atop");
	s_blend_mode_map[kMCGBlendModeDestinationOver] = MCNAME("destination over");
	s_blend_mode_map[kMCGBlendModeDestinationIn] = MCNAME("destination in");
	s_blend_mode_map[kMCGBlendModeDestinationOut] = MCNAME("destination out");
	s_blend_mode_map[kMCGBlendModeDestinationAtop] = MCNAME("destination atop");
	s_blend_mode_map[kMCGBlendModeXor] = MCNAME("xor");
	s_blend_mode_map[kMCGBlendModePlusDarker] = MCNAME("plus darker");
	s_blend_mode_map[kMCGBlendModePlusLighter] = MCNAME("plus lighter");
	s_blend_mode_map[kMCGBlendModeMultiply] = MCNAME("multiply");
	s_blend_mode_map[kMCGBlendModeScreen] = MCNAME("screen");
	s_blend_mode_map[kMCGBlendModeOverlay] = MCNAME("overlay");
	s_blend_mode_map[kMCGBlendModeDarken] = MCNAME("darken");
	s_blend_mode_map[kMCGBlendModeLighten] = MCNAME("lighten");
	s_blend_mode_map[kMCGBlendModeColorDodge] = MCNAME("color dodge");
	s_blend_mode_map[kMCGBlendModeColorBurn] = MCNAME("color burn");
	s_blend_mode_map[kMCGBlendModeSoftLight] = MCNAME("soft light");
	s_blend_mode_map[kMCGBlendModeHardLight] = MCNAME("hard light");
	s_blend_mode_map[kMCGBlendModeDifference] = MCNAME("difference");
	s_blend_mode_map[kMCGBlendModeExclusion] = MCNAME("exclusion");
	s_blend_mode_map[kMCGBlendModeHue] = MCNAME("hue");
	s_blend_mode_map[kMCGBlendModeSaturation] = MCNAME("saturation");
	s_blend_mode_map[kMCGBlendModeColor] = MCNAME("color");
	s_blend_mode_map[kMCGBlendModeLuminosity] = MCNAME("luminosity");
	
	s_transform_matrix_keys[0] = MCNAME("0,0");
	s_transform_matrix_keys[1] = MCNAME("1,0");
	s_transform_matrix_keys[2] = MCNAME("2,0");
	s_transform_matrix_keys[3] = MCNAME("0,1");
	s_transform_matrix_keys[4] = MCNAME("1,1");
	s_transform_matrix_keys[5] = MCNAME("2,1");
	s_transform_matrix_keys[6] = MCNAME("0,2");
	s_transform_matrix_keys[7] = MCNAME("1,2");
	s_transform_matrix_keys[8] = MCNAME("2,2");

	s_effect_type_map[kMCCanvasEffectTypeColorOverlay] = MCNAME("color overlay");
	s_effect_type_map[kMCCanvasEffectTypeInnerShadow] = MCNAME("inner shadow");
	s_effect_type_map[kMCCanvasEffectTypeOuterShadow] = MCNAME("outer shadow");
	s_effect_type_map[kMCCanvasEffectTypeInnerGlow] = MCNAME("inner glow");
	s_effect_type_map[kMCCanvasEffectTypeOuterGlow] = MCNAME("outer glow");
	
	s_effect_property_map[kMCCanvasEffectPropertyColor] = MCNAME("color");
	s_effect_property_map[kMCCanvasEffectPropertyBlendMode] = MCNAME("blend mode");
	s_effect_property_map[kMCCanvasEffectPropertyOpacity] = MCNAME("opacity");
	s_effect_property_map[kMCCanvasEffectPropertySize] = MCNAME("size");
	s_effect_property_map[kMCCanvasEffectPropertySpread] = MCNAME("spread");
	s_effect_property_map[kMCCanvasEffectPropertyDistance] = MCNAME("distance");
	s_effect_property_map[kMCCanvasEffectPropertyAngle] = MCNAME("angle");
	
	s_gradient_type_map[kMCGGradientFunctionLinear] = MCNAME("linear");
	s_gradient_type_map[kMCGGradientFunctionRadial] = MCNAME("radial");
	s_gradient_type_map[kMCGGradientFunctionSweep] = MCNAME("conical");
	s_gradient_type_map[kMCGLegacyGradientDiamond] = MCNAME("diamond");
	s_gradient_type_map[kMCGLegacyGradientSpiral] = MCNAME("spiral");
	s_gradient_type_map[kMCGLegacyGradientXY] = MCNAME("xy");
	s_gradient_type_map[kMCGLegacyGradientSqrtXY] = MCNAME("sqrtxy");
	
	s_canvas_fillrule_map[kMCGFillRuleEvenOdd] = MCNAME("even odd");
	s_canvas_fillrule_map[kMCGFillRuleNonZero] = MCNAME("non zero");
	
	s_image_filter_map[kMCGImageFilterNone] = MCNAME("none");
	s_image_filter_map[kMCGImageFilterLow] = MCNAME("low");
	s_image_filter_map[kMCGImageFilterMedium] = MCNAME("medium");
	s_image_filter_map[kMCGImageFilterHigh] = MCNAME("high");
	
	// TODO - confirm these command names
	s_path_command_map[kMCGPathCommandMoveTo] = MCNAME("move");
	s_path_command_map[kMCGPathCommandLineTo] = MCNAME("line");
	s_path_command_map[kMCGPathCommandQuadCurveTo] = MCNAME("quad");
	s_path_command_map[kMCGPathCommandCubicCurveTo] = MCNAME("cubic");
	s_path_command_map[kMCGPathCommandCloseSubpath] = MCNAME("close");
	s_path_command_map[kMCGPathCommandEnd] = MCNAME("end");
	
/* UNCHECKED */
}

void MCCanvasStringsFinalize()
{
	for (uint32_t i = 0; i < kMCGBlendModeCount; i++)
		MCValueRelease(s_blend_mode_map[i]);
	
	for (uint32_t i = 0; i < 9; i++)
		MCValueRelease(s_transform_matrix_keys[i]);

	for (uint32_t i = 0; i < _MCCanvasEffectTypeCount; i++)
		MCValueRelease(s_effect_type_map[i]);
	
	for (uint32_t i = 0; i < _MCCanvasEffectPropertyCount; i++)
		MCValueRelease(s_effect_property_map[i]);
	
	for (uint32_t i = 0; i < kMCGGradientFunctionCount; i++)
		MCValueRelease(s_gradient_type_map[i]);
	
	for (uint32_t i = 0; i < kMCGFillRuleCount; i++)
		MCValueRelease(s_canvas_fillrule_map[i]);
		
	for (uint32_t i = 0; i < kMCGImageFilterCount; i++)
		MCValueRelease(s_image_filter_map[i]);
	
	for (uint32_t i = 0; i < kMCGPathCommandCount; i++)
		MCValueRelease(s_path_command_map[i]);
}

template <typename T, int C>
bool _mcenumfromstring(MCNameRef *N, MCStringRef p_string, T &r_value)
{
	for (uint32_t i = 0; i < C; i++)
	{
		if (MCStringIsEqualTo(p_string, MCNameGetString(N[i]), kMCStringOptionCompareCaseless))
		{
			r_value = (T)i;
			return true;
		}
	}
	
	return false;
}

template <typename T, int C>
bool _mcenumtostring(MCNameRef *N, T p_value, MCStringRef &r_string)
{
	if (p_value >= C)
		return false;
	
	if (N[p_value] == nil)
		return false;
	
	r_string = MCValueRetain(MCNameGetString(N[p_value]));
    return true;
}

bool MCCanvasBlendModeFromString(MCStringRef p_string, MCGBlendMode &r_blend_mode)
{
	return _mcenumfromstring<MCGBlendMode, kMCGBlendModeCount>(s_blend_mode_map, p_string, r_blend_mode);
}

bool MCCanvasBlendModeToString(MCGBlendMode p_blend_mode, MCStringRef &r_string)
{
	return _mcenumtostring<MCGBlendMode, kMCGBlendModeCount>(s_blend_mode_map, p_blend_mode, r_string);
}

bool MCCanvasGradientTypeFromString(MCStringRef p_string, MCGGradientFunction &r_type)
{
	return _mcenumfromstring<MCGGradientFunction, kMCGGradientFunctionCount>(s_gradient_type_map, p_string, r_type);
}

bool MCCanvasGradientTypeToString(MCGGradientFunction p_type, MCStringRef &r_string)
{
	return _mcenumtostring<MCGGradientFunction, kMCGGradientFunctionCount>(s_gradient_type_map, p_type, r_string);
}

bool MCCanvasEffectTypeToString(MCCanvasEffectType p_type, MCStringRef &r_string)
{
	return _mcenumtostring<MCCanvasEffectType, _MCCanvasEffectTypeCount>(s_effect_type_map, p_type, r_string);
}

bool MCCanvasEffectTypeFromString(MCStringRef p_string, MCCanvasEffectType &r_type)
{
	return _mcenumfromstring<MCCanvasEffectType, kMCGFillRuleCount>(s_effect_type_map, p_string, r_type);
}

bool MCCanvasFillRuleToString(MCGFillRule p_fill_rule, MCStringRef &r_string)
{
	return _mcenumtostring<MCGFillRule, kMCGFillRuleCount>(s_canvas_fillrule_map, p_fill_rule, r_string);
}

bool MCCanvasFillRuleFromString(MCStringRef p_string, MCGFillRule &r_fill_rule)
{
	return _mcenumfromstring<MCGFillRule, kMCGFillRuleCount>(s_canvas_fillrule_map, p_string, r_fill_rule);
}

bool MCCanvasImageFilterToString(MCGImageFilter p_filter, MCStringRef &r_string)
{
	return _mcenumtostring<MCGImageFilter, kMCGImageFilterCount>(s_image_filter_map, p_filter, r_string);
}

bool MCCanvasImageFilterFromString(MCStringRef p_string, MCGImageFilter &r_filter)
{
	return _mcenumfromstring<MCGImageFilter, kMCGImageFilterCount>(s_image_filter_map, p_string, r_filter);
}

bool MCCanvasPathCommandToString(MCGPathCommand p_command, MCStringRef &r_string)
{
	return _mcenumtostring<MCGPathCommand, kMCGPathCommandCount>(s_path_command_map, p_command, r_string);
}

bool MCCanvasPathCommandFromString(MCStringRef p_string, MCGPathCommand &r_command)
{
	return _mcenumfromstring<MCGPathCommand, kMCGPathCommandCount>(s_path_command_map, p_string, r_command);
}

////////////////////////////////////////////////////////////////////////////////