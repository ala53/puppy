using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using This_may_explode_in_my_face.Core;

namespace This_may_explode_in_my_face.Images
{
    /*
     * Each image is made up of a series of regions - large blocks of pixel data (32x32 -- 1024 pixels)
     * Those regions are made up, in turn, of small blocks: 4x4 pixels (16 pixels)
     *
     * Each region is structured as such:
     *      There are 8 blocks along the x axis and 8 blocks along the y axis for a total of 64 chunks
     *          Neighboring blocks often share similar data and can be represented by their neighbor block's data
     *          A region begins with a simple structure: a table representing the data state of each block
     *          2 bits are used per block, for a total of 128 bits of data for the so-called "block table":
     *              0 = block has its own data
     *              1 = block shares data with the block to its left
     *              2 = block shares data with block above
     *              3 = block shares data with block to right
     *          The block below is ignored to reduce data requirements: a fifth state would require 3 bits per block instead of two
     *          The block table is immediately followed by raw block data in a scan order from top left to bottom right, row wise. Any 
     *          blocks marked as sharing data are not written to the stream.
     *      Thus, the region uses the space of up to 66 blocks to represent 64 blocks, but in practice much less as neighboring blocks
     *      can be highly similar (e.g. in a flat color image, a region is represented by only 3 blocks of data)
     *
     * Each of those blocks are made up of 2 color samples stored in 8-4-4 format: 
     *      8 bit Y (luminosity), 4 bit U and 4 bit V (chroma).
     *      and 2 bits per pixel: 0-3 describe how to blend between color 1 and color 2:
     *          0 = 100% color 1
     *          1 = 66% color 1, 33% color 2
     *          2 = 33% color 1, 66% color 2
     *          3 = 100% color 2
     *
     *
     * That means each "block" takes up:
     *      32 bits = 16 bits * 2 for color samples
     *      32 bits = 2 pixels * 16 bits for pixel blending
     *      --------
     *      64 bits per block (compared to 384 bits uncompressed -- 16 px * 24bpp)
     *
     * And each region:
     *      128 bits = 2 bits * 64 blocks for block table
     *      64 bits - 4096 bits = 64 bits * (1 to 64 blocks) for color samples 
     *      ----------
     *      192 bits - 4224 bits per region
     * 
     */
    public class CompressedImage
    {
        /// <summary>
        /// The region data for the image
        /// </summary>
        public Region[,] Regions { get; private set; }
        /// <summary>
        /// Creates a new compressed image. For best results, width and height should both be multiples of 32.
        /// </summary>
        /// <param name="width"></param>
        /// <param name="height"></param>
        public CompressedImage(int width, int height)
        {
            //Get the lower bound of the width and height
            width = ((int)Math.Floor(width / (float)Region.Width)) * Region.Width;
            height = ((int)Math.Floor(height / (float)Region.Height)) * Region.Height;
            Regions = new Region[width, height];
        }

        /// <summary>
        /// Compares this image to a previous one and returns a list of regions which have changed.
        /// </summary>
        /// <param name="last"></param>
        /// <returns>A list of changed regions. True means changed. False means same.</returns>
        public Array2D<bool> Compare(CompressedImage last, int comparisonThreshold = 8)
        {

        }
    }
}
