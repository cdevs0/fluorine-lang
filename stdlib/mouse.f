# Unknown Mouse module: native input operations are provided by the interpreter.
class Mouse {
    fn move(x, y) { return mouse_move(x, y); }
    fn left_click() { return mouse_left_click(); }
    fn right_click() { return mouse_right_click(); }
    fn middle_click() { return mouse_middle_click(); }
    fn left_click_at(x, y) { return mouse_left_click_at(x, y); }
    fn right_click_at(x, y) { return mouse_right_click_at(x, y); }
    fn position() { return mouse_position(); }
    fn scroll(amount) { return mouse_scroll(amount); }
    fn scroll_up(amount) { return mouse_scroll(amount); }
    fn scroll_down(amount) { return mouse_scroll(-amount); }
    fn set_system_cursor(cur) { return mouse_set_system_cursor(cur); }
	fn restore_system_cursor(cur) { return mouse_restore_system_cursor(); }	
}
