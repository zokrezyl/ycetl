{
  description = "Project using shared clang dev shell from GitHub";


  outputs = { self, clang-flake }: {
    nixpkgs = clang-flake.nixpkgs;
    devShells.x86_64-linux.default = clang-flake.devShells.x86_64-linux.default;
  };
}

