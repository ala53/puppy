using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace This_may_explode_in_my_face.Images
{
    /// <summary>
    /// Represents a YUV color in 4:2:2 format
    /// </summary>
    public struct YUVColor
    {
        public const int
            ColorDepthBits = 16, // Must be power of two -- do not change
            SizeBits = ColorDepthBits,
            SizeBytes = SizeBits / 8,
            LuminosityBits = ColorDepthBits / 2,
            ChromaBits = ColorDepthBits / 2,
            ChromaBitsU = ChromaBits / 2,
            ChromaBitsV = ChromaBitsU;
        /// <summary>
        /// The luminosity or grayscale color
        /// </summary>
        public readonly byte Y;
        /// <summary>
        /// The UV or chroma property of the color. We interthread the two like so (bit level shown):
        /// (x's represent stored values)
        /// U: x0x0x0x0
        /// V: x0x0x0x0
        /// UV: uvuvuvuv
        /// </summary>
        public readonly byte UV;
        /// <summary>
        /// Gets the "U" property alone: UV & 0xAA 
        /// </summary>
        public byte U => (byte)(UV & 0xAA); //Unweave the U
        /// <summary>
        /// Gets the "V" property alone: (UV & 0x55) << 1
        /// </summary>
        public byte V => (byte)((UV & 0x55) << 1); //Unweave the V

        public YUVColor(byte y, byte u, byte v)
        {
            //0b10101010 == 0xAA
            //0b01010101 == 0x55
            Y = y;
            UV = (byte)((u & 0xAA) | ((v >> 1) & 0x55));
        }

        public static YUVColor FromRGB(Microsoft.Xna.Framework.Color color) =>
            FromRGB(color.R, color.G, color.B);

        public static YUVColor FromRGB(float r, float g, float b)
        {
            //Actually YCbCr but whatever
            //From http://www.equasys.de/colorconversion.html
            float y = 0.257f * r + 0.504f * g + 0.098f * b + 16;
            float u = -0.148f * r + -0.291f * g + 0.439f * b + 128;
            float v = 0.439f * r + -0.368f * g + -0.071f * b + 128;
            return new YUVColor((byte)y, (byte)u, (byte)v);
        }

        public Microsoft.Xna.Framework.Color ToRGB()
        {
            float y = Y - 16;
            float u = U - 128;
            float v = V - 128;

            float r = 1.164f * y + 1.596f * v;
            float g = 1.164f * y + -0.392f * u + -0.813f * v;
            float b = 1.164f * y + 2.017f * u;

            return new Microsoft.Xna.Framework.Color(r, g, b);
        }
    }
}
