# Homebrew formula for msgpack-tools
# Maintainer: Nicholas Fraser <nicholas.rd.fraser@gmail.com>
class MsgpackTools < Formula
  desc "Command-line tools for converting between MessagePack and JSON"
  homepage "https://github.com/ludocode/msgpack-tools"
  url "https://github.com/ludocode/msgpack-tools/releases/download/v0.5/msgpack-tools-0.5.tar.gz"
  version "0.5"
  sha256 "6f382a74fd8715a8a0e6a2561a9f10b16db7604b3139486908b4054709f81d08"

  depends_on "cmake" => :build

  def install
    system "./configure", "--prefix=#{prefix}"
    system "make", "install"
  end
end
