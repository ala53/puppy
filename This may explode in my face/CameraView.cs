using DotImaging;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Numerics;
using System.Runtime.CompilerServices;
using System.Text;
using System.Threading.Tasks;
using This_may_explode_in_my_face.Core;

namespace This_may_explode_in_my_face
{
    public class CameraView
    {
        /// <summary>
        /// The size of a single chunk of a frame
        /// </summary>
        public const int ChunkSize = 24;
        /// <summary>
        /// The number of bytes in a chunk
        /// </summary>
        public const int ChunkContentLength = ChunkSize * ChunkSize;
        /// <summary>
        /// The maximum age of a chunk before it is forcibly regenerated
        /// </summary>
        public static readonly TimeSpan MaximumChunkAge = TimeSpan.FromSeconds(10);
        /// <summary>
        /// A small area of a frame which can be updated independently from any others
        /// </summary>
        public unsafe struct ImageRegion
        {
            /// <summary>
            /// The time since this chunk was last updated
            /// </summary>
            public TimeSpan TimeSinceLastChunkUpdate;
            /// <summary>
            /// A 16x16 grayscale chunk of an image
            /// </summary>
            public fixed byte Contents[ChunkContentLength];
            /// <summary>
            /// The offset in chunk space of this chunk
            /// </summary>
            public int OffsetX;
            /// <summary>
            /// The offset in chunk space of this chunk
            /// </summary>
            public int OffsetY;
            /// <summary>
            /// The offset in pixel space of this chunk
            /// </summary>
            public int PixelOffsetX => OffsetX * ChunkSize;
            /// <summary>
            /// The offset in pixel space of this chunk
            /// </summary>
            public int PixelOffsetY => OffsetY * ChunkSize;
            /// <summary>
            /// Whether the chunk has been marked as changed
            /// </summary>
            public bool Changed;
        }

        /// <summary>
        /// The width of the image
        /// </summary>
        public int Width { get; private set; }
        /// <summary>
        /// The height of the image
        /// </summary>
        public int Height { get; private set; }
        /// <summary>
        /// The number of chunks which make up the width
        /// </summary>
        public int ChunksWidth => (int)Math.Floor((double)Width / ChunkSize);
        /// <summary>
        /// The number of chunks which make up the height
        /// </summary>
        public int ChunksHeight => (int)Math.Floor((double)Height / ChunkSize);
        /// <summary>
        /// Gets the number of chunks changed in the last frame update
        /// </summary>
        public int ChunksChanged
        {
            get
            {
                var chunks = _nextFrameChunkList;
                int w = ChunksWidth, h = ChunksHeight;
                int i = 0;
                for (int x = 0; x < w; x++)
                    for (int y = 0; y < h; y++)
                        if (chunks[x, y].Changed) i++;
                return i;
            }
        }

        /// <summary>
        /// Gets the total number of chunks in the image
        /// </summary>
        public int ChunkCount => _nextFrameChunkList.Count;
        /// <summary>
        /// (in byte scale: 0-255) The maximum difference in intensity between the same pixel on two frames before the chunk is updated
        /// </summary>
        public int ColorDifferenceThresholdPerPixel { get; set; } = 24;
        /// <summary>
        /// (in byte scale: 0-255) The maximum difference in intensity between the all pixels on two frames before the chunk is updated
        /// </summary>
        public int ColorDifferenceThresholdPerChunk { get; set; } = 1536 + 256;

        /// <summary>
        /// The set of image chunks which make up the current frame.
        /// </summary>
        public Array2D<ImageRegion> CurrentFrame { get; private set; }
        //The cached array to break down the next frame
        private Array2D<ImageRegion> _nextFrameChunkList;

        private CameraCapture _cameraHandle;
        private Bgr<byte>[,] _rawFrame;
        private byte[,] _rawGrayData;

        private Stopwatch _stopwatchForChunkAges;
        /// <summary>
        /// Creates a new camera view
        /// </summary>
        /// <param name="width"></param>
        /// <param name="height"></param>
        /// <param name="frameRate"></param>
        /// <param name="cameraId"></param>
        public CameraView(int width = 640, int height = 480, int frameRate = 10, int cameraId = 0)
        {
            //Create and configure camera
            _cameraHandle = new CameraCapture(cameraId);
            _cameraHandle.FrameRate = frameRate;
            _cameraHandle.FrameSize = new DotImaging.Primitives2D.Size(width, height);
            //Open the view
            _cameraHandle.ConvertRgb = false;
            _cameraHandle.Open();
            Width = _cameraHandle.FrameSize.Width;
            Height = _cameraHandle.FrameSize.Height;
            //Create the raw frame cache
            _rawFrame = new Bgr<byte>[Width, Height]; // Don't ask...just...don't
            _rawGrayData = new byte[Height, Width];
            //Create chunk cache
            CurrentFrame = new Array2D<ImageRegion>(ChunksWidth, ChunksHeight);
            _nextFrameChunkList = new Array2D<ImageRegion>(ChunksWidth, ChunksHeight);
            //Fix the chunk cache so the first frame will work (otherwise, offsets will not be set)
            for (int x = 0; x < CurrentFrame.XSize; x++)
                for (int y = 0; y < CurrentFrame.YSize; y++)
                    CurrentFrame[x, y] = new ImageRegion() { OffsetX = x, OffsetY = y };
            //And turn on the chunk age tracker
            _stopwatchForChunkAges = Stopwatch.StartNew();
            //And read the first frame
            CaptureFrame();
        }

        /// <summary>
        /// Captures a frame and returns the chunked frame (also storing it in CurrentFrame)
        /// </summary>
        public Array2D<ImageRegion> CaptureFrame()
        {
            CaptureFrameInternal();
            ChunkFrame();
            FindChangedChunks();
            UpdateChunkAges();
            return CurrentFrame;
        }

        /// <summary>
        /// Captures a frame of video and converts it to grayscale
        /// </summary>
        private unsafe void CaptureFrameInternal()
        {
            //Images are stored row wise
            // 0,0 1,0 2,0
            // 0,1 1,1 2,1

            _cameraHandle.ReadTo(ref _rawFrame);
            for (int x = 0; x < _rawFrame.GetLength(0); x++)
                for (int y = 0; y < _rawFrame.GetLength(1); y++)
                {
                    var src = _rawFrame[x, y];
                    _rawGrayData[x, y] = (byte)((src.R + src.G + src.B) / 3);
                }
        }

        /// <summary>
        /// Breaks a frame into requisite chunks
        /// </summary>
        private unsafe void ChunkFrame()
        {
            var chunks = _nextFrameChunkList;
            //For performance, we will turn these into locals (otherwise, they can't be register stored)
            int w = ChunksWidth, h = ChunksHeight;
            //And turn it into a local too, for fast access

            //Outer loop: iterate through chunks
            for (int x = 0; x < w; x++)
                for (int y = 0; y < h; y++)
                {
                    //byte* imageData = (byte*)image.l;
                    var chunk = new ImageRegion() { OffsetX = x, OffsetY = y };
                    //Inner loop: copy pixels
                    for (int i = 0; i < ChunkSize; i++) //y axis
                    {
                        int pixelX = x * ChunkSize, pixelY = y * ChunkSize;
                        //Compute which row to copy into (in the chunk)
                        byte* rowStart = chunk.Contents + (i * ChunkSize);
                        fixed (byte* imageData = &_rawGrayData[pixelY + i, pixelX])
                        Buffer.MemoryCopy(imageData, rowStart, ChunkSize, ChunkSize);
                    }

                    //Store the chunk
                    chunks[x, y] = chunk;
                }
        }

        /// <summary>
        /// Adds to the ages of the chunks based on the data from the stopwatch.
        /// </summary>
        private void UpdateChunkAges()
        {
            int w = ChunksWidth, h = ChunksHeight;
            var currentFrame = CurrentFrame;
            var arr = currentFrame.Backing;
            for (int x = 0; x < w; x++)
                for (int y = 0; y < h; y++)
                    arr[currentFrame.To1DOffset(x, y)].TimeSinceLastChunkUpdate += _stopwatchForChunkAges.Elapsed;

            _stopwatchForChunkAges.Restart();
        }

        /// <summary>
        /// Finds which chunks have changed between the last frame and this one and moves them into the current frame object
        /// </summary>
        private void FindChangedChunks()
        {
            int w = ChunksWidth, h = ChunksHeight;
            var currentFrame = CurrentFrame;
            var nextFrame = _nextFrameChunkList;
            for (int x = 0; x < w; x++)
                for (int y = 0; y < h; y++)
                {
                    //To avoid an extra copy, we do a bit of complicated
                    var chunk = currentFrame[x, y];
                    //Update if the chunk has changed or it is over a certain age
                    bool shouldUpdate = !AreChunksSimilar(chunk, nextFrame[x, y]) || chunk.TimeSinceLastChunkUpdate > MaximumChunkAge;
                    nextFrame.Backing[nextFrame.To1DOffset(x, y)].Changed = shouldUpdate;

                    //Put the new chunk in the old one's place
                    if (shouldUpdate)
                        currentFrame[x, y] = nextFrame[x, y];
                    else currentFrame.Backing[currentFrame.To1DOffset(x, y)].Changed = false; //Or unmark the change flag
                }
        }

        /// <summary>
        /// Compares two chunks for their similarity
        /// </summary>
        /// <param name="a"></param>
        /// <param name="b"></param>
        /// <returns>Whether the chunk similarity is close enough to keep the old chunk</returns>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public unsafe bool AreChunksSimilar(ImageRegion a, ImageRegion b)
        {
            int totalDiff = 0;
            for (int i = 1; i < ChunkContentLength - 1; i++)
            {
                //int threePixels = (a.Contents[i - 1] - b.Contents[i - 1]) + (a.Contents[i] - b.Contents[i]) + (a.Contents[i + 1] - b.Contents[i + 1]);
                //int diff = /*Math.Abs(a.Contents[i] - b.Contents[i])*/ Math.Abs(threePixels / 3);
                int diff = Math.Min(Math.Min(Math.Abs(a.Contents[i] - b.Contents[i - 1]), Math.Abs(a.Contents[i] - b.Contents[i])), Math.Abs(a.Contents[i] - b.Contents[i + 1]));
                totalDiff += diff;
                if (diff > ColorDifferenceThresholdPerPixel) return false;
            }
            if (totalDiff > ColorDifferenceThresholdPerChunk) return false;
            return true;
        }
    }
}
