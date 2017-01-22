# Homebrew formula for msgpack-tools develop branch. This builds and installs
# the latest development code rather than the latest stable release.
#
# The msgpack-tools latest stable release formula lives at:
#
#     https://ludocode.github.io/msgpack-tools.rb
#
# Maintainer: Nicholas Fraser <nicholas.rd.fraser@gmail.com>
class MsgpackToolsDev < Formula
  desc "Command-line tools for converting between MessagePack and JSON"
  homepage "https://github.com/ludocode/msgpack-tools"
  url "https://github.com/ludocode/msgpack-tools/archive/develop.tar.gz"
  version "develop"

  depends_on "cmake" => :build

  def install
    system "./configure", "--prefix=#{prefix}"
    system "make"
    system "env", "CTEST_OUTPUT_ON_FAILURE=1", "make", "test"
    system "make", "install"
  end

  test do
    system "json2msgpack", "-v"
    system "msgpack2json", "-v"
    system "bash", "-o", "pipefail", "-c", 'echo "{\"Hello\": \"world!\"}" | json2msgpack | msgpack2json -d'
  end
end
