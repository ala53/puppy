using System;
using System.Collections;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;

namespace This_may_explode_in_my_face.Core
{
    /// <summary>
    /// A class to represent a 2 dimensional array with the ability to both track the backing data's size (x,y) and access it as a 1 dimensional array.
    /// </summary>
    /// <typeparam name="T"></typeparam>
    public class Array2D<T> : IEnumerable<T>, IReadOnlyList<T>
    {
        private T[] _backing;
        public T[] Backing => _backing;
        public int Length => _backing.Length;
        public int Count => _backing.Length;
        /// <summary>
        /// The X size of the array
        /// </summary>
        public int XSize { get; private set; }
        /// <summary>
        /// The Y size of the array
        /// </summary>
        public int YSize { get; private set; }
        public Array2D(int sizex, int sizey)
        {
            XSize = sizex;
            YSize = sizey;
            _backing = new T[(sizex * sizey)];
        }

        /// <summary>
        /// Creates a 2D array with an existing array
        /// backing it.
        /// </summary>
        /// <param name="array"></param>
        /// <param name="sizex"></param>
        /// <param name="sizey"></param>
        public Array2D(T[] array, int sizex, int sizey)
        {
            XSize = sizex;
            YSize = sizey;
            _backing = array;
            if (Length != sizex * sizey) throw new ArgumentException("Size mismatch: sizex * ysize does not equal array length!");
        }

        public unsafe Array2D(T[,] array)
        {
            XSize = array.GetLength(0);
            YSize = array.GetLength(1);
            //I hate this, but we have to do a copy...
            //Unless we want to drop into unsafe code (and store pointers)
            //...which we cannot do anyway for some stupid reasons
            _backing = new T[XSize * YSize];

            for (int x = 0; x < XSize; x++)
                for (int y = 0; y < YSize; y++)
                    _backing[To1DOffset(x, y)] = array[x, y];
        }

        public void ForEach(Action<int, int, T> handler)
        {
            for (int x = 0; x < XSize; x++)
                for (int y = 0; y < YSize; y++)
                    handler(x, y, this[x, y]);
        }

        public void Populate(Func<int, int, T> handler)
        {
            for (int x = 0; x < XSize; x++)
                for (int y = 0; y < YSize; y++)
                    this[x, y] = handler(x, y);
        }

        /// <summary>
        /// Converts a 2D offset to a 1D offset in the array
        /// </summary>
        /// <param name="x"></param>
        /// <param name="y"></param>
        /// <returns></returns>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public int To1DOffset(int x, int y) => (y * XSize) + x;
        public void To2DOffset(int oneDOffset, out int x, out int y)
        {
            y = oneDOffset / XSize;
            x = oneDOffset - (y * XSize);
        }
        /// <summary>
        /// Checks whether an X, Y pair is within the bounds of the array
        /// </summary>
        /// <param name="x"></param>
        /// <param name="y"></param>
        /// <returns></returns>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public bool InBounds(int x, int y)
        {
            //Inlined from To1DOffset(x, y) >= 0 && To1DOffset(x, y) < _backing.Length;
            int off = (y * XSize) + x;
            return off >= 0 && off < _backing.Length;
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public T ValueOrDefault(int x, int y)
        {
            // Inlined from:
            //if (InBounds(x, y)) return this[x, y];
            int off = (y * XSize) + x;
            return off >= 0 && off < _backing.Length ? _backing[off] : default(T);
        }

        public struct StructEnumerator : IEnumerator<T>
        {
            int offset;
            T[] arr;

            public T Current => arr[offset];

            object IEnumerator.Current => arr[offset];

            public StructEnumerator(T[] backing)
            {
                arr = backing;
                offset = 0;
            }

            public void Dispose() { }

            public bool MoveNext()
            {
                offset++;
                return offset < arr.Length;
            }

            public void Reset() => offset = 0;
        }

        public StructEnumerator GetEnumerator() => new StructEnumerator(_backing);

        IEnumerator<T> IEnumerable<T>.GetEnumerator() => GetEnumerator();

        IEnumerator IEnumerable.GetEnumerator() => GetEnumerator();

        public T this[int x, int y]
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get { return _backing[(y * XSize) + x]; } //Inlined from _backing[To1DOffset(x, y)];
            [MethodImpl(MethodImplOptions.AggressiveInlining)] /*Some weirdness -- the function isn't correctly inlined normally*/
            set { _backing[(y * XSize) + x] = value; } //Inlined from _backing[To1DOffset(x, y)] = value;
        }

        public T this[int index]
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get { return _backing[index]; }
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            set { _backing[index] = value; }
        }
    }
}
