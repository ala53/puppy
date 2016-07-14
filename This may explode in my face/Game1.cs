using Microsoft.Xna.Framework;
using Microsoft.Xna.Framework.Graphics;
using Microsoft.Xna.Framework.Input;
using System;
using This_may_explode_in_my_face.Core;
using DotImaging;
using System.Diagnostics;
using System.Threading.Tasks;
using System.Collections.Generic;

namespace This_may_explode_in_my_face
{
    /// <summary>
    /// This is the main type for your game.
    /// </summary>
    public class Game1 : Game
    {
        GraphicsDeviceManager graphics;
        SpriteBatch spriteBatch;
        CameraView view;
        Array2D<Texture2D> textureCache;
        byte[] _uploadCache;
        bool drawUpdateNoise;
        int tickRate = 200;

        public Game1()
        {
            graphics = new GraphicsDeviceManager(this);
            Content.RootDirectory = "Content";
            IsFixedTimeStep = false;
            this.IsMouseVisible = true;
            // this.TargetElapsedTime = TimeSpan.FromSeconds(0.5);
        }

        /// <summary>
        /// Allows the game to perform any initialization it needs to before starting to run.
        /// This is where it can query for any required services and load any non-graphic
        /// related content.  Calling base.Initialize will enumerate through any components
        /// and initialize them as well.
        /// </summary>
        protected override void Initialize()
        {
            // TODO: Add your initialization logic here

            base.Initialize();
        }

        /// <summary>
        /// LoadContent will be called once per game and is the place to load
        /// all of your content.
        /// </summary>
        protected override void LoadContent()
        {
            // Create a new SpriteBatch, which can be used to draw textures.
            spriteBatch = new SpriteBatch(GraphicsDevice);
            view = new CameraView(frameRate: 3);// 320, 240, 5);
            textureCache = new Array2D<Texture2D>(view.ChunksWidth, view.ChunksHeight);
            textureCache.Populate((x, y) => new Texture2D(GraphicsDevice, CameraView.ChunkSize, CameraView.ChunkSize, false, SurfaceFormat.Alpha8));
            _uploadCache = new byte[CameraView.ChunkContentLength];
            UpdateTextureCache(true);

            //And asynchronously update the chunks on screen
            Task.Run(() =>
            {
                Stopwatch sw = Stopwatch.StartNew();
                while (true)
                {
                    sw.Restart();

                    view.CaptureFrame();

                    int remainingTime = tickRate - (int)sw.ElapsedMilliseconds;
                    if (remainingTime < tickRate && remainingTime > 0)
                        System.Threading.Thread.Sleep(remainingTime);
                }
            });
        }

        private unsafe void UpdateTextureCache(bool first = false)
        {
            textureCache.ForEach((x, y, texture) =>
            {
                var chunk = view.CurrentFrame[x, y];
                if (!first && !chunk.Changed) return;
                for (int i = 0; i < CameraView.ChunkContentLength; i++)
                    _uploadCache[i] = chunk.Contents[i];

                texture.SetData(_uploadCache);
            });
        }

        /// <summary>
        /// UnloadContent will be called once per game and is the place to unload
        /// game-specific content.
        /// </summary>
        protected override void UnloadContent()
        {
            // TODO: Unload any non ContentManager content here
        }

        /// <summary>
        /// Allows the game to run logic such as updating the world,
        /// checking for collisions, gathering input, and playing audio.
        /// </summary>
        /// <param name="gameTime">Provides a snapshot of timing values.</param>
        protected override void Update(GameTime gameTime)
        {
            if (GamePad.GetState(PlayerIndex.One).Buttons.Back == ButtonState.Pressed || Keyboard.GetState().IsKeyDown(Keys.Escape))
                Exit();

            if (Keyboard.GetState().IsKeyDown(Keys.D1))
                view.ColorDifferenceThresholdPerPixel--;
            if (Keyboard.GetState().IsKeyDown(Keys.D2))
                view.ColorDifferenceThresholdPerPixel++;

            if (Keyboard.GetState().IsKeyDown(Keys.D3))
                view.ColorDifferenceThresholdPerChunk -= 32;
            if (Keyboard.GetState().IsKeyDown(Keys.D4))
                view.ColorDifferenceThresholdPerChunk += 32;

            if (Keyboard.GetState().IsKeyDown(Keys.D5))
                tickRate++;
            if (Keyboard.GetState().IsKeyDown(Keys.D6))
                tickRate--;

            if (Keyboard.GetState().IsKeyDown(Keys.D7))
                drawUpdateNoise = !drawUpdateNoise;

            // TODO: Add your update logic here

            base.Update(gameTime);

            Stopwatch sw = Stopwatch.StartNew();
            Window.Title =
                "Total Chunks: " + view.ChunkCount +
                ", Changed: " + view.ChunksChanged +
                ", Before: " + (((double)view.Width * view.Height * 10) / 1024 / 1024).ToString("N2") + "mb/s" +
                ", After: " + (((double)view.ChunksChanged * CameraView.ChunkContentLength * 10) / 1024 / 1024).ToString("N2") + "mb/s" +
                ", Per pixel: " + view.ColorDifferenceThresholdPerPixel + ", Per chunk: " + view.ColorDifferenceThresholdPerChunk +
                ", FPS: " + (1000f / tickRate).ToString("N2");

            UpdateTextureCache();
        }

        /// <summary>
        /// This is called when the game should draw itself.
        /// </summary>
        /// <param name="gameTime">Provides a snapshot of timing values.</param>
        protected override void Draw(GameTime gameTime)
        {
            GraphicsDevice.Clear(Color.CornflowerBlue);

            // TODO: Add your drawing code here
            spriteBatch.Begin();
            textureCache.ForEach((x, y, texture) =>
            {
                var color = Color.White;
                if (drawUpdateNoise && view.CurrentFrame[x, y].Changed) color = Color.Green;
                spriteBatch.Draw(texture, new Rectangle(x * CameraView.ChunkSize, y * CameraView.ChunkSize, CameraView.ChunkSize, CameraView.ChunkSize), color);
            });

            spriteBatch.End();

            base.Draw(gameTime);
        }
    }
}
