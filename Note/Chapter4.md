# Synchronizing concurrent operations

同步并发操作。



这章覆盖的内容：

1.等待一个事件

2.等待一次性(one-off)的事件，使用futures

3.等待一个时间限制

4.使用同步操作去简化代码



有时候不仅需要去保护数据，同时也要**同步不同线程上的操作。**



一个线程可能需要**去等待另一个线程完成一个任务**，在第一个线程可以完成它自己任务的时候。



通常，那是普遍的，想让一个线程去等待一个确定的事件发生或者一个条件为true。

即使这是可能的完成，通过定时地检查一个"task complete"flag，或者一些相似的东西在共享数据里面，这也是离理想情况很远的。



C++标准库提高了一些机制来处理这个，**通过condition variables和futures的形式。**



这个章节，会讨论如何去等待一个event，使用条件变量和futures。

**并且使用它们来简化同步操作。**



# Waiting for an event or other condition



如果一个线程等待另一个线程完成任务，它有一些选择。

第一，检查一个flag在共享数据上(被mutex保护)，然后让另一个线程去设置这个flag，当任务完成的时候。这在两方面有浪费：线程消耗了宝贵的访问时间，重复地检查flag，当互斥量被等待线程锁定的时候，它不能被其它任何线程锁定。

这两种方式都影响了线程的等待，因为他们限制了一个线程等待的可用的资源，并且阻止了它去设置那个flag。这个类似(akin)于在夜晚保持清醒，然后和火车司机交谈：他不得不把火车开的慢一点，

因为你使他分心了，因此他花费了更长的时间到达了这里。

**同样地，等待线程消耗了可以被另一个线程使用的系统资源，并且可能导致等待更长。**



另一种选择是等待线程睡眠一段时间，**在检查和使用std::this_thread::sleep_for()函数之间。**

```c++
bool flag;
std::mutex m;

void wait_for_flag()
{
	std::unique_lock<std::mutex> lk(m);
	
	while(!flag)
	{
		lk.unlock();
		std::this_thread::sleep_for(std::chrono::milliseconds(100));//休眠100毫秒
		lk.lock();
	}
}
```



在这个循环里面，**函数解锁了互斥量在睡眠之前**，并且在之后锁定了它，所以另一个线程有机会请求它，

并且设置这个flag。



这是一个巨大的提升，因为线程不会浪费处理时间，当它睡眠的时候，**但是这是非常困难地设置睡眠时间。**

太短的睡眠时间会浪费处理时间的检查，太长的睡眠时间，线程会持续睡眠，即使它等待的线程已经完成了任务，导致一些些延迟。



第三种，更推荐的，去使用C++标准库的机制，**去等待一个事件。**

最基础的机能用来等待一个事件，被另一个线程所触发的是**条件变量。**

概念上地，一个条件变量被联系到一些事件或者其它的条件，并且一个或者多个线程可以等待这个条件变量直到满足，然后，它可以**notify**一个或多个等待该条件的线程，以便唤醒它们并允许它们继续处理。



## Waiting for a condition with condition variables

使用条件变量等待一个条件。



C++标准库提供了两个条件变量的实现：std::condition_variable和std::condition_variable_any。

它们都声明在&lt;condition_variable&gt;库头文件中。

两种情况下，它们都需要和mutex一起工作，目的是为了提供正确的同步，前面的受限于只能和std::mutex一起工作，鉴于后者可以任何一起工作，但是需要符合一些最小的标准，使得看起来像mutex，因此有着_any后缀。



因为std::condition_variable_any更加的通用，这里会有额外的成本，依据大小来看，性能，

或者操作系统资源，那么std::condition_variable应当更受欢迎，除非额外的便利性被需要的话。



使用条件变量的例子如下：

![image-20220504131411522](../Image/4.1.png)

首先，有一个队列，用来在两个线程之间传递数据。当数据准备好了之后，线程准备数据，并且锁定互斥量，

保护队列，使用std::lock_guard，并且压入数据到队列中。然后，它调用notify_one()成员函数在条件变量实例，

**去通知等待线程。**



你还有一个处理线程。这个线程首先锁定了互斥量，但是使用了std::unique_lock而不是std::_lock_guard，

等会，你将看到为什么。线程然后调用wait()在std::condition_variable，传递了锁对象还有一个lambda表达式，表示条件变量等待的东西。



wait的实现检查了条件，通过调用提供的lambda函数，并且返回它是否满足，如果条件不满足，

wait解锁mutex，并且将线程阻塞或者变成等待状态。

当条件变量被激活，通过一个调用notify_one()，线程将会被唤醒，**重新请求锁定mutex，并且再次检查**

**条件，从wait返回，并且mutex仍然被锁定。**



这也是为什么你需要std::unique_lock而不是std::lock_guard，**等待线程必须解锁mutex，**

等待线程必须解锁mutex，当它在之后等待和再一次锁定的时候，std::lock_guard不提供这个便利性。

如果mutex保持锁定，当线程睡眠的时候，data-preparation线程不能够去锁定mutex，去添加一个item到队列中，并且等待线程将永远看不到条件满足。



在调用wait的时候，一个条件变量可能会检查条件的条件任意多次，然而，它总是在锁定互斥锁的情况下执行此

操作，并且只有当用于测试条件的函数返回true时，才会立即返回。



当等待线程请求mutex，并且检查条件的时候，如果它不是在直接的回复，对于另一个线程的通知，

叫做**虚假唤醒(spurious wak)**。因为任何这样的虚假定义的数量和频率被定义不确定的，不建议使用一个有负面影响的函数对于条件的检查。



std::unique_lock的便利性，不仅用于wait，它也用于一旦你有数据去处理，但是在处理它之前。

处理数据是一个消耗时间的操作，持有一个lock在一个mutex上过长时间是不理想的。



持有一个队列在线程之间转移数据是非常寻常的情况。

同步可以受限在队列它本身，**它减少了同步的大量问题和竞争条件。**



std::unqiue_lock相比于std::lock_guard，提供了解锁的操作，析构函数会判断解锁了没有，如果解锁了，就不再解锁，否则就和std::lock_guard一样，**粒度更细。**



# Building a thread-safe queue with condition variables

构建一个线程安全的队列，使用条件变量。



3.2.3那个是线程安全的栈。



以std::queue&lt;&gt;为适配器的形式如下：

![image-20220507100630123](../Image/4.2.png)

如果你忽略了构造函数，赋值运算符和交互操作，你只剩下三组操作：这些操作询问了队列的状态

(empty()和size())，这些询问了队列的元素(front()和back())，还有这些修改了队列(push(), pop()和emplace())。



就像3.2.3的栈，并且因此你有同样的问题，关于固有的竞争条件，在接口上。

因此，你需要合并front()和pop()到一个函数调用中，就像你合并top()和pop()对于栈来说。

4.1添加了一个细微的差别，当使用一个队列去传递数据在线程之间，接受队列经常需要去等待数据。

让我们提供两个变种在pop()上:try_pop()，它尝试去弹出值从queue，但是立马返回一个值(使用一个暗示失败的值)即使这里没有值可以获取，还有wait_and_pop()，会去等待，直到这里有个值可以获取。

![image-20220507101743256](../Image/4.2.1.png)

try_pop和wait_for_pop有两个版本。

try_pop的第一个重载版本存储了返回的值在引用的变量上，因此，它可以使用返回的值作为状态，

它返回了true如果它返回一个值，如果失败，返回false。

第二个版本的重载不会这么做，因为它直接返回了值，但是返回的指针可以是null，如果这里没有值可以获取。



它和前面那个queue有什么关系，你可以抽取出代码，对于push和wait_and_pop在这里。

![image-20220507104039558](../Image/4.4.png)

![image-20220507104117044](../Image/4.4.1.png)

std::unique_lock相比于std::lock_guard的优势，就是，可以在中途释放锁。



可以看到，mutex和条件变量现在包含在threadsafe_queue的实例中，因此分离的变量不再被请求，

没有外部同步的操作被请求，对于调用push来说。

wait_and_pop在内部处理了条件变量的等待。



其它的重载是很容易写的。



最终版本如下：

```c++
	template<typename T>
	class threadsafe_queue
	{
	private:
		mutable std::mutex mut;//mutex must be mutable
		std::queue<T> data_queue;
		std::condition_variable data_cond;
	public:
		threadsafe_queue() {}
		threadsafe_queue(const threadsafe_queue& other)
		{
			std::lock_guard<std::mutex> lk(other.mut);
			data_queue = other.data_queue;
		}
		threadsafe_queue& operator=(const threadsafe_queue&) = delete;//disallow assignment for simplicity

		void push(T new_value)
		{
			std::lock_guard<std::mutex> lk(mut);
			data_queue.push(new_value);
			data_cond.notify_one();//notify
		}

		bool try_pop(T& value)
		{
			std::lock_guard<std::mutex> lk(mut);
			if (data_queue.empty())
				return false;
			value = data_queue.front();
			data_queue.pop();
			return true;
		}
		std::shared_ptr<T> try_pop()
		{
			std::lock_guard<std::mutex> lk(mut);
			if (data_queue.empty())
				return false;
			std::shared_ptr<T> res(std::make_shared<T>(data_queue.front()));
			data_queue.pop();
			return res;
		}

		void wait_and_pop(T& value)
		{
			//std::unqiue_lock could release in halfway
			std::unique_lock<std::mutex> lk(mut);
			data_cond.wait(lk, [this] {return !data_queue.empty(); });
			value = data_queue.front();
			data_queue.pop();
		}

		std::shared_ptr<T> wait_and_pop()
		{
			std::unique_lock<std::mutex> lk(mut);
			data_cond.wait(lk, [this] {return !data_queue.empty(); });
			std::shared_ptr<T> res(std::make_shared<T>(data_queue.front()));
			data_queue.pop();
			return res;
		}
		bool empty() const
		{
			std::lock_guard<std::mutex> lk(mut);
			return data_queue.empty();
		}
	};
```



即使empty()是一个const成员函数，并且其它的参数对于拷贝构造来说，是一个const引用，

其它线程也可能具有non-const的引用对于这个对象，并且调用可变的成员函数，因此我们需要去锁定mutex。



自从锁定mutex是一个可变的操作，因此mutex对象必须是mutable的，**因此它可以被锁定在empty和拷贝构造中。**

也就是说，拷贝构造的参数是一个const引用，但是我们想使用它的mutex，mutex的锁定是可变操作，因此我们需要给它一个修饰符mutable。



//75













