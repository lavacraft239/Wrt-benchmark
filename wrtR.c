#[link(name = "wrt")] // <- Compilá wrt.c a libwrt.a
extern "C" {
    fn wrt_attack(url: *const u8, requests: i32, parallel: i32);
}

fn main() {
    let url = b"https://example.com\0";
    unsafe {
        wrt_attack(url.as_ptr(), 100, 10);
    }
}
