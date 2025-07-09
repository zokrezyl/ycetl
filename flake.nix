{
  description = "Flake with a custom dev shell combining nix.dev and LLVM tooling";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixpkgs-unstable";
  };

  outputs = { self, nixpkgs, ... }: let
    # Import nixpkgs for x86_64-linux
    pkgs = import nixpkgs { system = "x86_64-linux"; };
  in {
    # Create a custom "default" package which is actually a dev shell
    packages.x86_64-linux = {
      default = pkgs.llvmPackages.libcxxStdenv.mkDerivation {
        name = "clang-libcxx-nix-shell";
        buildInputs = [
          pkgs.llvmPackages.clang
          pkgs.llvmPackages.clang-tools
          pkgs.llvmPackages.lld
          pkgs.gcc
          pkgs.go
          pkgs.curl
          pkgs.git
          #pkgs.glibc
          #pkgs.glibc.dev
          pkgs.cmake
          pkgs.pkg-config
          pkgs.python314
          pkgs.boost.dev
          pkgs.lldb
          pkgs.lua
          pkgs.nodejs
          pkgs.bear
          pkgs.entr
          pkgs.fzf
          pkgs.gh
          pkgs.zsh
          pkgs.neovim
          pkgs.nix.dev       # provides nix-expr-c headers & pc
        ];
        # Optional: drop into the nix.dev provided shell entrypoint
        shellHook = ''
          echo "Entering custom nix-expr-c + LLVM shell"
          export TERM=xterm-256color
          #zsh
          # If you prefer the nix.dev dev-shell entrypoint:
          # exec ${pkgs.nix.dev}/bin/dev-shell
        '';
      };
    };

    # Point defaultPackage to our custom shell
    defaultPackage.x86_64-linux = self.packages.x86_64-linux.default;

    # Also expose it as the devShell
    devShells.x86_64-linux = {
      default = self.packages.x86_64-linux.default;
    };
  };
}

