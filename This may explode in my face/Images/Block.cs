using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;

namespace This_may_explode_in_my_face.Images
{
    //Chunks are unsafe for comparison performance
    [StructLayout(LayoutKind.Explicit)]
    public unsafe struct Block
    {
        public const int
            Width = 4, // Must be power of two -- do not change
            Height = 4, // Must be power of two -- do not change
            RowSizeBits = Width * 2,
            RowSizeBytes = RowSizeBits / 8,
            PixelDataLengthBits = RowSizeBits * Height,
            PixelDataLengthBytes = PixelDataLengthBits / 8,
            SizeBits = (Width * Height * 2) + (YUVColor.ColorDepthBits * 2), // W*H*2bpp + 2*colors
            SizeBytes = SizeBits / 8;
        //The first 32 bits are color data in Y:U:V format
        /// <summary>
        /// The first color representing the block
        /// </summary>
        [FieldOffset(0)]
        public YUVColor Color1;
        /// <summary>
        /// The second color representing the block
        /// </summary>
        [FieldOffset(YUVColor.SizeBytes)]
        public YUVColor Color2;
        //Following that is the pixel information
        /// <summary>
        /// The start of the image's pixel information
        /// </summary>
        [FieldOffset(2 * YUVColor.SizeBytes)]
        public fixed byte PixelData[PixelDataLengthBytes];
        /// <summary>
        /// The first row of pixel data
        /// </summary>
        [FieldOffset(2 * YUVColor.SizeBytes)]
        public fixed byte Row0[RowSizeBytes];
        /// <summary>
        /// The first row of pixel data
        /// </summary>
        [FieldOffset(2 * YUVColor.SizeBytes + RowSizeBytes)]
        public fixed byte Row1[RowSizeBytes];
        /// <summary>
        /// The first row of pixel data
        /// </summary>
        [FieldOffset(2 * YUVColor.SizeBytes + 2 * RowSizeBytes)]
        public fixed byte Row2[RowSizeBytes];
        /// <summary>
        /// The first row of pixel data
        /// </summary>
        [FieldOffset(2 * YUVColor.SizeBytes + 3 * RowSizeBytes)]
        public fixed byte Row3[RowSizeBytes];
        
        public Block()
        {

        }
    }
}
