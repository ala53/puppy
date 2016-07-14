using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace This_may_explode_in_my_face.Images
{
    public unsafe struct Region
    {
        public const int
            Width = 32,
            Height = 32,
            BlocksPerRow = Width / Block.Width,
            BlocksPerColumn = Height / Block.Height,
            BlockCount = BlocksPerRow * BlocksPerColumn,
            SizeBits = BlockCount * 2 + Block.SizeBits * BlockCount,
            SizeBytes = SizeBits / 8;

        public fixed Block Blocks[BlockCount];
    }
}
