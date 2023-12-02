{ lib
, stdenv
, hyprland
,
}:
stdenv.mkDerivation {
  pname = "hypershell";
  version = "0.1";
  src = ./.;

  inherit (hyprland) nativeBuildInputs;

  buildInputs = [ hyprland ] ++ hyprland.buildInputs;

  meta = with lib; {
    homepage = "https://github.com/killown/hypershell";
    description = "client overview hyprland plugin";
    license = licenses.mit;
    platforms = platforms.linux;
  };
}
