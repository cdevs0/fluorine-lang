
namespace convert {
    // Float conversions
    fn ftoi(value) {
        if (value < 0) {
            return ceil(value);
        }
        return floor(value);
    }

    fn ftos(value) {
        return to_string(value);
    }

    fn ftob(value) {
        return value != 0;
    }

    fn ftor(value) {
        return round(value);
    }

    fn ftof(value) {
        return to_number(value);
    }

    fn ftofloor(value) {
        return floor(value);
    }

    fn ftoceil(value) {
        return ceil(value);
    }

    // Integer conversions
    fn itof(value) {
        return to_number(value);
    }

    fn itos(value) {
        return to_string(value);
    }

    fn itob(value) {
        return value != 0;
    }

    fn itofloor(value) {
        return floor(value);
    }

    fn itoceil(value) {
        return ceil(value);
    }

    fn itoround(value) {
        return round(value);
    }

    // General number conversions
    fn ntof(value) {
        return to_number(value);
    }

    fn ntoi(value) {
        return ftoi(to_number(value));
    }

    fn ntos(value) {
        return to_string(value);
    }

    fn ntob(value) {
        return to_number(value) != 0;
    }

    // String conversions
    fn stos(value) {
        return to_string(value);
    }

    fn stof(value) {
        return to_number(value);
    }

    fn stoi(value) {
        return ftoi(to_number(value));
    }

    fn stob(value) {
        let text = lower(trim(to_string(value)));

        if (text == "true" || text == "yes" || text == "on" || text == "1") {
            return true;
        }

        if (text == "false" || text == "no" || text == "off" || text == "0" || text == "") {
            return false;
        }

        return false;
    }

    // Boolean conversions
    fn btos(value) {
        if (value) {
            return "true";
        }
        return "false";
    }

    fn btoi(value) {
        if (value) {
            return 1;
        }
        return 0;
    }

    fn btof(value) {
        return btoi(value);
    }

    fn btob(value) {
        return value == true;
    }

    // Unicode character conversions
    fn ctoi(value) {
        return ord(to_string(value));
    }

    fn ctof(value) {
        return to_number(to_string(value));
    }

    fn ctos(value) {
        return to_string(value);
    }

    fn ctob(value) {
        return to_string(value) != "";
    }

    fn itoc(value) {
        return chr(ftoi(value));
    }

    fn ftoc(value) {
        return chr(ftoi(value));
    }

    fn stoc(value) {
        return chr(ftoi(to_number(value)));
    }

    // String and character utilities
    fn to_chars(value) {
        return ascii_array(to_string(value));
    }

    fn to_char(value) {
        return chr(ftoi(to_number(value)));
    }

    fn char_code(value) {
        return ord(to_string(value));
    }

    fn code_char(value) {
        return chr(ftoi(value));
    }

    fn upper_string(value) {
        return upper(to_string(value));
    }

    fn lower_string(value) {
        return lower(to_string(value));
    }

    fn trimmed_string(value) {
        return trim(to_string(value));
    }

    fn split_string(value, separator) {
        return split(to_string(value), to_string(separator));
    }

    fn join_strings(values, separator) {
        return join(values, to_string(separator));
    }

    fn string_length(value) {
        return len(to_string(value));
    }

    // Generic conversions
    fn to_num(value) {
        return to_number(value);
    }

    fn to_str(value) {
        return to_string(value);
    }

    fn to_int(value) {
        return ftoi(to_number(value));
    }

    fn to_float(value) {
        return to_number(value);
    }

    fn to_bool(value) {
        if (value == null || value == false || value == 0 || value == "") {
            return false;
        }
        return true;
    }

    fn to_array(value) {
        if (type_of(value) == "array") {
            return value;
        }
        return [value];
    }

    fn to_map(value) {
        if (type_of(value) == "map") {
            return value;
        }
        return {"value": value};
    }

    fn type(value) {
        return type_of(value);
    }

    fn is_number(value) {
        return is_number(value);
    }

    fn is_string(value) {
        return type_of(value) == "string";
    }

    fn is_boolean(value) {
        return type_of(value) == "boolean";
    }

    fn is_array(value) {
        return type_of(value) == "array";
    }

    fn is_map(value) {
        return type_of(value) == "map";
    }
}
