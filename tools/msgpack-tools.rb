# Homebrew formula for msgpack-tools
# Maintainer: Nicholas Fraser <nicholas.rd.fraser@gmail.com>
class MsgpackTools < Formula
  desc "Command-line tools for converting between MessagePack and JSON"
  homepage "https://github.com/ludocode/msgpack-tools"
  url "https://github.com/ludocode/msgpack-tools/releases/download/v0.4/msgpack-tools-0.4.tar.gz"
  version "0.4"
  sha256 "6ca77477ed47ccf4ac882ace3ee5f33a7bc7b929d12c5f10fa0c8fb5874fbc10"

  depends_on "cmake" => :build

  def install
    system "./configure", "--prefix=#{prefix}"
    system "make", "install"
  end
end
