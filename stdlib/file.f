class File {
    let name = "";
    let encoding = "UTF-8";

    // The instance's encoding is used by default. Unknown strings remain UTF-8
    // internally; the native file functions convert at the file boundary.
    fn write(text) {
        return write_text(this.name, text, this.encoding);
    }

    fn write_as(text, encoding) {
        return write_text(this.name, text, encoding);
    }

    fn append(text) {
        return append_text(this.name, text, this.encoding);
    }

    fn append_as(text, encoding) {
        return append_text(this.name, text, encoding);
    }

    fn read() {
        return read_text(this.name, this.encoding);
    }

    fn read_as(encoding) {
        return read_text(this.name, encoding);
    }

    fn exists() {
        return file_exists(this.name);
    }

    fn remove() {
        return remove_file(this.name);
    }

    fn size() {
        return file_size(this.name);
    }

    fn copy(destination) {
        return copy_file(this.name, destination);
    }

    fn rename(destination) {
        let result = rename_file(this.name, destination);
        if (result) {
            this.name = destination;
        }
        return result;
    }

    fn make_directory() {
        return make_dir(this.name);
    }

    fn is_directory() {
        return is_dir(this.name);
    }
}
