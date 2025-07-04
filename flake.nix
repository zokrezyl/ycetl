{
  description = "Clang + libc++ + CMake dev shell";

  inputs.nixpkgs.url = "github:NixOS/nixpkgs";

  outputs = { self, nixpkgs }: {
    devShells = {
      x86_64-linux = let
        pkgs = import nixpkgs {
          system = "x86_64-linux";
        };
        llvm = pkgs.llvmPackages;
      in {
        default = llvm.libcxxStdenv.mkDerivation {
          name = "clang-libcxx-shell";
          buildInputs = [
            llvm.clang
            llvm.lld
            pkgs.cmake
          ];
        };
      };
    };
  };
}

