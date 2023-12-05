{ lib
, stdenv
, hyprland
,
}:
stdenv.mkDerivation {
  pname = "hyprshell";
  version = "0.1";
  src = ./.;

  inherit (hyprland) nativeBuildInputs;

  buildInputs = [ hyprland ] ++ hyprland.buildInputs;

  meta = with lib; {
    homepage = "https://github.com/killown/hyprshell";
    description = "client overview hyprland plugin";
    license = licenses.mit;
    platforms = platforms.linux;
  };
}
