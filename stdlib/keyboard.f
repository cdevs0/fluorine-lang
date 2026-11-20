# Unknown Keyboard module: native input operations are provided by the interpreter.
class Keyboard {
    fn type_text(text) { return keyboard_type_text(text); }
    fn press(key) { return keyboard_press(key); }
    fn key_down(key) { return keyboard_key_down(key); }
    fn key_up(key) { return keyboard_key_up(key); }
}
