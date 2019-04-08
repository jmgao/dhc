## dhc

dhc is a DirectInput/XInput wrapper DLL that implements controller hotplugging
for games that don't do so themselves. It pretends that there are always a
fixed number of PS4 controllers plugged in at once, and maps the inputs of real
controllers onto the virtual controllers as they are plugged in.

### Installation

Download [the latest release](https://github.com/jmgao/dhc/releases) and place
the appropriate DLLs in the same folder as the game you want to use dhc with.
Note that the game binary might be not be the one that's at the top level of
the game's directory: if you don't see dhc.log or dhc.toml files being created
after you start the game, you probably chose the wrong one. You can right click
on the game in Task Manager and use properties to find the location of the
actual binary.

### Compatibility

See the [wiki](https://github.com/jmgao/dhc/wiki/Compatibility) for a table of
tested games and controllers.

### Configuration

On first launch, dhc will create a `dhc.toml` file in the same directory as
`dhc.dll`, which you can edit with a text editor to change settings.

### Compiling

dhc is implemented in both C++ and rust, so you'll need working toolchains for
both languages. Only cross-compilation from Linux has been tested, so if you
want to build on a Windows host, you'll either have fun figuring it out, or
just use WSL or a Linux VM.

Install the dependencies:
```sh
sudo apt-get install ninja-build
sudo apt-get install python3-pip && pip3 install meson
sudo apt-get install mingw-w64

# Install rust
curl https://sh.rustup.rs -sSf | sh -s -- -y
# Restart your shell or source your profile to make sure rustup and cargo are on your $PATH

# Add rust's cross compilation targets
rustup target add i686-pc-windows-gnu x86_64-pc-windows-gnu

# Install cbindgen
cargo install cbindgen
```

And then build:
```
git clone https://github.com/jmgao/dhc.git
cd dhc
./build/generate.sh
ninja -C build/i686 install && ninja -C build/x86_64 install
```

The built DLLs will be copied into `dist/{i686, x86_64}`.

### Known issues

- XInput controllers only get their triggers forwarded as digital buttons, not
  analog values.

- Steam's controller emulation will put itself on top of dhc, which will result
  in controllers showing up multiple times. To avoid this, either launch the
  game with no controllers connected, or disable Steam's controller support
  (View > Settings > Controller > General Controller Settings > uncheck everything).

### Reporting bugs

File a bug [here](https://github.com/jmgao/dhc/issues/new), with your `dhc.log`
file attached. Make sure not to restart the game after noticing a crash, as the
log file is deleted and recreated on every launch.
