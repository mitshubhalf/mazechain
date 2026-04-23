{pkgs}: {
  deps = [
    pkgs.wget
    pkgs.curl
    pkgs.curl.dev
    pkgs.psmisc
    pkgs.mailutils
    pkgs.asio
    pkgs.pkg-config
    pkgs.cmake
    pkgs.gcc
    pkgs.boost
    pkgs.openssl
    pkgs.gnumake
  ];
}