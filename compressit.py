import zlib

fp = open("compressed.bin", "wb+")

fp.write( zlib.compress(b'Hello Zlib! Thanks from bringing me back from the land of the deflated!', 0) )

fp.close()


