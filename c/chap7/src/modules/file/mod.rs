pub struct File(String);

impl File {
    pub fn to_str(&self) -> &str {
        if self.0 == "chapter1.md" {
            "# Getting started\n"
        } else if self.0 == "chapter2.md" {
            "# Wrapping up\n"
        } else {
            panic!("No content defined for file: {}", self.0);
        }
    }
}

pub fn read_file(filename: &str) -> File {
    File(filename.to_owned())
}
