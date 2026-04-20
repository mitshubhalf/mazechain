{pkgs}: {
  deps = [
    pkgs.mailutils
    pkgs.asio
    pkgs.pkg-config
    pkgs.cmake
    pkgs.gcc
    pkgs.boost
    pkgs.openssl
  ];
}
