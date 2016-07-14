using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading;
using System.Threading.Tasks;

namespace This_may_explode_in_my_face.Core
{
    /// <summary>
    /// A simple threadpool with cooperative multitasking and priority scheduling
    /// </summary>
    public static class PrioritizingCooperativeThreadpool
    {
        public class WorkItem
        {
            public Action<WorkItem> ToExecute { get; private set; }
            private int _prior;
            public int Priority { get { return _prior; } set { _prior = value; ReorderItem(this); } }
            public bool IsRunning { get; private set; }
            public bool CancellationRequested { get; private set; }
            public WorkItem(Action<WorkItem> toExec, int priority)
            {
                Priority = priority;
                ToExecute = toExec;
            }
            /// <summary>
            /// Waits for an item to complete asynchronously
            /// </summary>
            public void WaitFor(WorkItem other)
            {
                //Make sure the other thread is higher priority than us
                //Because it might be running on the same system thread
                //Which makes <yield> calls completely useless

                while (other.IsRunning)
                {
                    if (other.Priority < Priority) other.Priority = Priority + 1;
                    Yield();
                }
            }

            /// <summary>
            /// Yields the current execution, returning true if the task should continue or 
            /// false if it should not.
            /// </summary>
            /// <returns></returns>
            public bool Yield()
            {
                BackgroundThreadInterruption(Priority);
                return CancellationRequested;
            }

            /// <summary>
            /// Signals the task to cancel itself
            /// </summary>
            public void Cancel()
            {
                Priority = int.MaxValue; //So we cancel immediately rather than wait for the queue
                CancellationRequested = true;
            }
        }
        private static List<WorkItem> _queue = new List<WorkItem>();
        private static object _queueLock = new object();
        private static Thread[] _threads;

        static PrioritizingCooperativeThreadpool()
        {
            _threads = new Thread[Environment.ProcessorCount * 2]; //Overprovision resources in case a thread sleeps
            for (int i = 0; i < _threads.Length; i++)
            {
                var t = new Thread(BackgroundThreadLoop);
                t.IsBackground = true;
                t.Priority = ThreadPriority.Lowest; //Background generation threads are not so important
                t.Start();
            }
        }

        /// <summary>
        /// The base loop for the background threads.
        /// </summary>
        private static void BackgroundThreadLoop()
        {
            while (true)
            {
                WorkItem item = null;
                lock (_queueLock)
                {
                    //Check if the queue has anything
                    if (_queue.Count > 0)
                    {
                        //Take the last item from the list
                        item = _queue[_queue.Count - 1];
                        _queue.RemoveAt(_queue.Count - 1);
                    }
                }
                //Check if it actually is an item
                if (item == null)
                    Thread.Sleep(5);
                //Execute if it is
                else item.ToExecute(item);
            }
        }

        private static void BackgroundThreadInterruption(int currentItemPriority)
        {
            //Check if there's a higher priority item in the queue
            WorkItem last = default(WorkItem);
            lock (_queueLock)
            {
                if (_queue.Count == 0) return; //Nothing in the queue
                last = _queue[_queue.Count - 1];
                if (last.Priority <= currentItemPriority)
                    return; //Not more important
                            //Remove it from queue so we can execute it
                _queue.RemoveAt(_queue.Count - 1);
            }

            //Execute it...
            if (last.ToExecute != null)
                last.ToExecute(last);
        }

        private static void ReorderItem(WorkItem item)
        {
            lock (_queueLock)
            {
                if (_queue.Contains(item))
                {
                    int currInd = _queue.IndexOf(item);
                    _queue.RemoveAt(currInd);
                    int whereToInsert = 0;
                    for (int i = 0; i < _queue.Count; i++)
                    {
                        if (_queue[i].Priority < item.Priority)
                            whereToInsert = i + 1;
                    }
                    _queue.Insert(whereToInsert, item);
                }
            }
        }

        /// <summary>
        /// Queues a new work item
        /// </summary>
        /// <param name="work">The work item, which accepts an action that returns control to the queueing program.</param>
        /// <param name="priority">The priority of the work scheduled. Higher is better.</param>
        public static WorkItem Queue(Action<WorkItem> work, int priority)
        {
            lock (_queueLock)
            {
                //Insert based on priority
                int whereToInsert = 0;
                for (int i = 0; i < _queue.Count; i++)
                {
                    if (_queue[i].Priority < priority)
                        whereToInsert = i + 1;
                }
                var item = new WorkItem(work, priority);
                _queue.Insert(whereToInsert, item);
                return item;
            }
        }
    }
}
