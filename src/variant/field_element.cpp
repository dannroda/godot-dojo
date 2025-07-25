//
// Created by hazel on 3/06/25.
//
#include "variant/field_element.h"

#include "tools/logger.h"

using namespace godot;

FieldElement::FieldElement()
= default;

FieldElement::FieldElement(const String& hex_str, size_t max_bytes)
{
    // Skip "0x" prefix if present
    size_t start_idx = (hex_str.substr(0, 2) == "0x") ? 2 : 0;

    // Calculate actual string length without prefix
    size_t hex_length = hex_str.length() - start_idx;

    // Handle odd number of characters by assuming leading zero
    bool is_odd = hex_length % 2 != 0;
    size_t num_bytes = (hex_length + is_odd) / 2;

    // Ensure we don't overflow the output buffer
    if (num_bytes > max_bytes)
    {
        return;
        //        throw std::runtime_error("Input hex string too long for output buffer");
    }

    size_t out_idx = 0;

    // Handle first nibble separately if we have odd number of characters
    if (is_odd)
    {
        String nibble = hex_str.substr(start_idx, 1);
        felt->data[out_idx++] = static_cast<uint8_t>(nibble.hex_to_int());
    }

    // Process two hex digits at a time
    for (size_t i = is_odd ? 1 : 0; i < hex_length; i += 2)
    {
        String byte_str = hex_str.substr(start_idx + i, 2);
        felt->data[out_idx++] = static_cast<uint8_t>(byte_str.hex_to_int());
    }
}

FieldElement::FieldElement(int enum_value)
{
    memset(felt->data, 0, 32);
    felt->data[31] = static_cast<uint8_t>(enum_value);
}

FieldElement::FieldElement(DOJO::FieldElement* existing_felt)
{
    felt = existing_felt;
}

FieldElement::FieldElement(DOJO::FieldElement existing_felt)
{
    felt = &existing_felt;
}

FieldElement::FieldElement(TypedArray<String>& addresses)
{
    for (int i = 0; i < addresses.size(); i++)
    {
        String address = addresses[i];
        if (address.length() == 42)
        {
            felt->data[i] = static_cast<uint8_t>(address.hex_to_int());
        }
        else
        {
            Logger::error("Invalid address: ", address);
        }
    }
}

FieldElement::~FieldElement()
{
}

PackedByteArray FieldElement::to_packed_array(const void* data, const int size)
{
    PackedByteArray _bytes;
    _bytes.resize(size);
    memcpy(_bytes.ptrw(), data, size);
    Logger::debug("[FieldElement] -> [PackedByteArray] ", _bytes.hex_encode());
    return _bytes;
}

dojo_bindings::FieldElement FieldElement::from_string(const String& hex_str, size_t max_bytes)
{
    DOJO::FieldElement result = {};

    memset(result.data, 0, max_bytes);

    size_t start_idx = (hex_str.substr(0, 2) == "0x") ? 2 : 0;
    size_t hex_length = hex_str.length() - start_idx;

    if (hex_length > 64)
    {
        Logger::error("Hex string too long for FieldElement");
        return result;
    }

    String padded_hex = hex_str.substr(start_idx);
    while (padded_hex.length() < 64)
    {
        padded_hex = "0" + padded_hex;
    }

    for (size_t i = 0; i < 64; i += 2)
    {
        String byte_str = padded_hex.substr(i, 2);
        int byte_value = byte_str.hex_to_int();
        if (byte_value < 0)
        {
            Logger::error("Invalid hex character in string");
            return result;
        }
        result.data[i / 2] = static_cast<uint8_t>(byte_value);
    }

    return result;
}

DOJO::FieldElement FieldElement::short_string_to_felt(const String& cairo_str)
{
    DOJO::ResultFieldElement resCairo = DOJO::cairo_short_string_to_felt(cairo_str.utf8().get_data());
    if (resCairo.tag == DOJO::ErrFieldElement)
    {
        Logger::error("Failed to convert string to FieldElement");
        return {};
    }
    DOJO::FieldElement felt = {GET_DOJO_OK(resCairo)};
    return felt;
}

DOJO::FieldElement FieldElement::nulled()
{
    return from_enum(0);
}

PackedByteArray FieldElement::nulled_as_bytes()
{
    return to_packed_array(nulled().data);
}

String FieldElement::nulled_as_string()
{
    return get_as_string_no_ptr(nulled());
}

PackedByteArray FieldElement::as_packed_array() const
{
    return to_packed_array(felt->data);
}

String FieldElement::to_string() const
{
    String ret = "0x";
    for (size_t i = 0; i < 32; i++)
    {
        ret += String::num_int64(felt->data[i], 16, false);
    };

    return ret;
}

const char* FieldElement::to_string_c_str() const
{
    return to_string().utf8().get_data();
}

DOJO::FieldElement FieldElement::from_enum(int enum_value)
{
    DOJO::FieldElement field_element = {};

    memset(field_element.data, 0, 32);
    field_element.data[31] = static_cast<uint8_t>(enum_value);


    return field_element;
}

String FieldElement::bytearray_deserialize(const uintptr_t& data_len = 32)
{
    DOJO::Resultc_char testing = DOJO::bytearray_deserialize(get_felt(), data_len);
    if (testing.tag == DOJO::Errc_char)
    {
        Logger::debug_extra("FieldElement", "Can't deserialize... Trying Cairo String");
        return parse_cairo();
    }
    Logger::success_extra("FieldElement", GET_DOJO_OK(testing));
    return {GET_DOJO_OK(testing)};
}

String FieldElement::get_as_string(DOJO::FieldElement* _felt)
{
    String ret = "0x";
    for (size_t i = 0; i < 32; i++)
    {
        ret += String::num_int64(_felt->data[i], 16, false);
    };

    return ret;
}

String FieldElement::get_as_string_no_ptr(DOJO::FieldElement _felt)
{
    String ret = "0x";
    for (size_t i = 0; i < 32; i++)
    {
        ret += String::num_int64(_felt.data[i], 16, false);
    };

    return ret;
}

DOJO::CArrayFieldElement FieldElement::bytearray_serialize(const String& msg)
{
    DOJO::ResultCArrayFieldElement result = DOJO::bytearray_serialize(msg.utf8().get_data());
    if (result.tag == DOJO::ErrCArrayFieldElement)
    {
        Logger::error("Failed to serialize string");
        return {};
    }
    return GET_DOJO_OK(result);
}

DOJO::FieldElement FieldElement::cairo_short_string_to_felt(const String& msg)
{
    DOJO::ResultFieldElement result = DOJO::cairo_short_string_to_felt(msg.utf8().get_data());
    if (result.tag == DOJO::ErrFieldElement)
    {
        Logger::error("Failed to convert string to FieldElement");
        return {};
    }
    return GET_DOJO_OK(result);
}

String FieldElement::parse_cairo()
{
    DOJO::Resultc_char resCairo = DOJO::parse_cairo_short_string(get_felt_no_ptr());
    if (resCairo.tag == DOJO::Errc_char)
    {
        Logger::debug_extra("FieldElement", "No cairo string found, returning as string hex");
        return {to_string()};
    }
    String result = GET_DOJO_OK(resCairo);
    return result;
}

std::vector<DOJO::FieldElement> FieldElement::create_array(TypedArray<String> array)
{
    std::vector<DOJO::FieldElement> result;
    result.resize(array.size());

    for (int i = 0; i < array.size(); i++)
    {
        String address = array[i];
        if (address.length() == 42)
        {
            DOJO::FieldElement field_elem = FieldElement::from_string(address);
            result[i] = field_elem;
        }
        else
        {
            Logger::error("Invalid address: ", address);
            result[i] = {};
        }
    }
    return result;
}
