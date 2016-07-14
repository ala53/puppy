using DotImaging;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace MurderBot
{
    class Program
    {
        public unsafe struct FrameChunk
        {
            public const int ChunkSize = 16;
            public const int ContentLength = ChunkSize * ChunkSize;
            /// <summary>
            /// A 16x16 grayscale chunk of an image
            /// </summary>
            public fixed byte Contents[ContentLength];
            public int OffsetX, OffsetY;
            /// <summary>
            /// Whether the chunk has been marked as changed
            /// </summary>
            public bool IsChanged;
        }
        const int frameWidth = 640, frameHeight = 480;
        public static FrameChunk[,] currentFrame = new FrameChunk[frameWidth / FrameChunk.ChunkSize, frameHeight / FrameChunk.ChunkSize];
        public static FrameChunk[,] nextFrameCachedArray = new FrameChunk[frameWidth / FrameChunk.ChunkSize, frameHeight / FrameChunk.ChunkSize];
        static void Main(string[] args)
        {
            CameraCapture reader = new CameraCapture();
            reader.FrameRate = 20;
            reader.FrameSize = new DotImaging.Primitives2D.Size(frameWidth, frameHeight);
            var frame1 = reader.ReadAs<Bgr<byte>>().ToGray();
            WriteToChunks(frame1, currentFrame);
            System.Threading.Thread.Sleep(1000);
            reader.Read();
            Stopwatch sw = Stopwatch.StartNew();
            var frame2 = reader.ReadAs<Bgr<byte>>().ToGray();
            WriteToChunks(frame2, nextFrameCachedArray);

            CompareChunks(currentFrame, nextFrameCachedArray);

            sw.Stop();

            frame1.Save("test.png");
            System.Threading.Thread.Sleep(1000);
            frame2.Save("test2.png");
            System.Threading.Thread.Sleep(1000);

            var outImg = frame2.ToBgr();

            for (int x = 0; x < frameWidth / FrameChunk.ChunkSize; x++)
                for (int y = 0; y < frameHeight / FrameChunk.ChunkSize; y++)
                {
                    if (!nextFrameCachedArray[x, y].IsChanged)
                        outImg.Draw(new DotImaging.Primitives2D.Rectangle(x * FrameChunk.ChunkSize, y * FrameChunk.ChunkSize, FrameChunk.ChunkSize, FrameChunk.ChunkSize), Bgr<byte>.Black, 1, 0);
                    else
                        outImg.Draw(new DotImaging.Primitives2D.Rectangle(x * FrameChunk.ChunkSize, y * FrameChunk.ChunkSize, FrameChunk.ChunkSize, FrameChunk.ChunkSize), Bgr<byte>.Red, 1, 0);
                }

            outImg.Save("test3.png");

            outImg = frame1.ToBgr();

            for (int x = 0; x < frameWidth / FrameChunk.ChunkSize; x++)
                for (int y = 0; y < frameHeight / FrameChunk.ChunkSize; y++)
                {
                    if (!nextFrameCachedArray[x, y].IsChanged)
                        outImg.Draw(new DotImaging.Primitives2D.Rectangle(x * FrameChunk.ChunkSize, y * FrameChunk.ChunkSize, FrameChunk.ChunkSize, FrameChunk.ChunkSize), Bgr<byte>.Black, 1, 0);
                    else
                        outImg.Draw(new DotImaging.Primitives2D.Rectangle(x * FrameChunk.ChunkSize, y * FrameChunk.ChunkSize, FrameChunk.ChunkSize, FrameChunk.ChunkSize), Bgr<byte>.Red, 1, 0);
                }

            outImg.Save("test4.png");

            int count = CountChanged(nextFrameCachedArray);
            Console.WriteLine("Total Chunks:    " + nextFrameCachedArray.Length);
            Console.WriteLine("Chunks Changed:  " + count);
            Console.WriteLine("MB/s before:     " + (((double)nextFrameCachedArray.Length * (FrameChunk.ContentLength) * 20) / 1024 / 1024).ToString("N1") + "mb");
            Console.WriteLine("MB/s after:      " + (((double)count * (FrameChunk.ContentLength) * 20) / 1024 / 1024).ToString("N1") + "mb");
            Console.WriteLine("Processing time: " + sw.ElapsedMilliseconds + "ms");
            Console.Read();
        }

        public static int CountChanged(FrameChunk[,] chunks)
        {
            int w = chunks.GetLength(0), h = chunks.GetLength(1);
            int i = 0;
            for (int x = 0; x < w; x++)
                for (int y = 0; y < h; y++)
                    if (chunks[x, y].IsChanged) i++;
            return i;
        }


        public unsafe static void WriteToChunks(Gray<byte>[,] image, FrameChunk[,] chunks)
        {
            int w = image.GetLength(1), h = image.GetLength(0);
            //Images are stored row wise
            // 0,0 1,0 2,0
            // 0,1 1,1 2,1
            //Outer loop: iterate chunks
            for (int x = 0; x < w; x += FrameChunk.ChunkSize)
                for (int y = 0; y < h; y += FrameChunk.ChunkSize)
                {
                    //byte* imageData = (byte*)image.l;
                    var chunk = new FrameChunk();
                    chunk.OffsetX = x / FrameChunk.ChunkSize;
                    chunk.OffsetY = y / FrameChunk.ChunkSize;
                    //Inner loop: copy pixels
                    for (int i = 0; i < FrameChunk.ChunkSize; i++) //y axis
                    {
                        //Compute which row to copy to
                        byte* rowStart = chunk.Contents + (i * FrameChunk.ChunkSize);
                        // number of rows down + offset in row
                        //imageData + ((y + i) * image.Size.Width) + x
                        //Buffer.MemoryCopy(image[x, y], rowStart, 16, 16);
                        for (int j = 0; j < FrameChunk.ChunkSize; j++)
                        {
                            rowStart[j] = image[y, x + j];
                        }
                    }

                    //Store the chunk
                    chunks[x / FrameChunk.ChunkSize, y / FrameChunk.ChunkSize] = chunk;
                }
        }

        /// <summary>
        /// Tracks which chunks of an image have changed between image A and image B
        /// </summary>
        /// <param name="chunksA"></param>
        /// <param name="chunksB"></param>
        /// <returns></returns>
        public static void CompareChunks(FrameChunk[,] chunksA, FrameChunk[,] chunksB)
        {
            int w = chunksA.GetLength(0), h = chunksA.GetLength(1);
            for (int x = 0; x < w; x++)
                for (int y = 0; y < h; y++)
                {
                    chunksB[x, y].IsChanged = !AreChunksSimilarEnough(chunksA[x, y], chunksB[x, y]);
                }
        }

        const int ColorDiffThresholdPerPixel = 12;
        //const int TotalColorDiffThreshold = 76800;
        public unsafe static bool AreChunksSimilarEnough(FrameChunk a, FrameChunk b)
        {
            int totalDiff = 0;
            for (int i = 1; i < FrameChunk.ContentLength - 1; i++)
            {
                int threePixels = (a.Contents[i - 1] - b.Contents[i - 1]) + (a.Contents[i] - b.Contents[i]) + (a.Contents[i + 1] - b.Contents[i + 1]);
                int diff = Math.Abs(threePixels / 3);
                totalDiff += diff;
                if (diff > ColorDiffThresholdPerPixel) return false;
            }
            //if (totalDiff > TotalColorDiffThreshold) return false;
            return true;
        }
    }
}
