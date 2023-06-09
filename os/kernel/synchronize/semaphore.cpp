#include <semaphore.hpp>
#include <pmm.hpp>
#include <kout.hpp>
#include <spinlock.hpp>
#include <mutex.hpp>
#include <interrupt.hpp>
#include <clock.hpp>

// 实现进程队列的部分

void ProcessQueueManager::print_all_queue(proc_queue& pq)
{
    if (pq.front == nullptr || pq.front->next == nullptr)
    {
        return;
    }
    ListNode* ptr = pq.front->next;
    while (ptr != nullptr)
    {
        pm.show(ptr->proc);
        ptr = ptr->next;
    }
}

void ProcessQueueManager::init(proc_queue& pq)
{
    pq.front = (ListNode*)kmalloc(sizeof(struct ListNode));
    pq.rear = pq.front;
    if (pq.front == nullptr)
    {
        kout[red] << "Process Queue Init Falied!" << endl;
        return;
    }
    pq.front->next = nullptr;
}

void ProcessQueueManager::destroy_pq(proc_queue& pq)
{
    ListNode* ptr = pq.front;
    while (ptr != nullptr)
    {
        ListNode* t = ptr->next;
        kfree(ptr);
        ptr = t;
    }
    pq.front = pq.rear = nullptr;
}

bool ProcessQueueManager::isempty_pq(proc_queue& pq)
{
    ListNode* ptr = pq.front;
    if (ptr == nullptr)
    {
        return true;
    }
    if (ptr->next == nullptr)
    {
        return true;
    }
    return false;
}

int ProcessQueueManager::length_pq(proc_queue& pq)
{
    ListNode* ptr = pq.front;
    if (ptr == nullptr || ptr->next == nullptr)
    {
        return 0;
    }
    ptr = ptr->next;
    int ret_len = 0;
    while (ptr != nullptr)
    {
        ret_len++;
        ptr = ptr->next;
    }
    return ret_len;
}

void ProcessQueueManager::enqueue_pq(proc_queue& pq, proc_struct* ins_proc)
{
    ListNode* newnode = (ListNode*)kmalloc(sizeof(struct ListNode));
    newnode->proc = ins_proc;
    pq.rear->next = newnode;
    pq.rear = pq.rear->next;
}

proc_struct* ProcessQueueManager::front_pq(proc_queue& pq)
{
    if (pq.front == pq.rear)
    {
        kout[yellow] << "The Process Queue is Empty!" << endl;
        return nullptr;
    }
    proc_struct* ret = pq.front->next->proc;
    return ret;
}

void ProcessQueueManager::dequeue_pq(proc_queue& pq)
{
    if (pq.front == pq.rear)
    {
        kout[yellow] << "The Process Queue is Empty!" << endl;
        return;
    }
    ListNode* t = pq.front->next;
    pq.front->next = t->next;
    if (pq.rear == t)pq.rear = pq.front;
    kfree(t);
}

ProcessQueueManager pqm;

/*------------------------------------------------*/

// 实现信号量的部分
void SEMAPHORE::init(int intr_val)
{
    if (intr_val < 0)
    {
        // 信号量的值不应该为负的
        kout[red] << "Semaphore's Value Init Should not be negative!" << endl;
        return;
    }
    value = intr_val;
    pqm.init(wait_pq);
}

int SEMAPHORE::wait(proc_struct* proc)
{
    // 关闭中断和加自旋锁都是为了保证信号量操作的原子性
    bool intr_flag;
    intr_save(intr_flag);
    spin_lock.lock();
    if (value <= 0)
    {
        // 当前发出wait信号的进程应该阻塞
        pqm.enqueue_pq(wait_pq, proc);
        // 在进程执行流上的改变大抵就是状态的改变
        // 调度器会自行区分
        // 事实上这里wait_queue的作用就是确定接触阻塞的顺序(公平的FIFO)
        // 这样强信号量的设计可以被证明可以保证互斥场景下不会饥饿的情况
        // 而如果不设计队列而只是改变状态这里就会可能出现饥饿的情况
        pm.wait_ref_proc(proc);
        pm.switchstat_proc(proc, Proc_sleeping);
        value--;
    }
    else
    {
        value--;
        // 接下来仍然是正常的时间片轮转调度的执行
    }
    spin_lock.unlock();
    intr_restore(intr_flag);
    if (value < 0)
    {
        // 中断开关的原因导致调度需要安排在这里
        // 立即触发调度器 从而实现立即的进程切换功能
        pm.imme_trigger_schedule();
    }
    return value;
}

void SEMAPHORE::signal(proc_struct* proc)
{
    bool intr_flag;
    intr_save(intr_flag);
    spin_lock.lock();
    if (value < 0)
    {
        // 会唤醒一个进程 即将一个进程从等待队列中移除
        // 这并不是立即执行的意思 所以这里不需要立即触发调度器
        proc_struct* proc = pqm.front_pq(wait_pq);
        pm.wait_unref_proc(proc);
        if (!pm.is_signal(proc))
        {
            // 说明还是不能够唤醒
            // 但是这个信号量上的进程队列需要被弹出
            value++;
            pqm.dequeue_pq(wait_pq);
        }
        else
        {
            // 可以被唤醒
            // 多一个状态的切换
            pqm.dequeue_pq(wait_pq);
            pm.switchstat_proc(proc, Proc_ready);
            value++;
        }
    }
    else
    {
        value++;
    }
    spin_lock.unlock();
    intr_restore(intr_flag);
}

bool SEMAPHORE::destroy()
{
    // 当这个信号量的进程队列上没有等待的进程时
    // 才可以销毁这个信号量的进程队列
    // if (pqm.isempty_pq(wait_pq))
    // {
    //     pqm.destroy_pq(wait_pq);
    //     return true;
    // }
    // else
    // {
    //     kout[red] << "The Semaphore's wait queue is NOT Empty!" << endl;
    //     return false;
    // }

    // 某些奇怪的逻辑
    // 不够当需要调用destroy时一定是对于单个进程的销毁了
    // 强制销毁即可
    pqm.destroy_pq(wait_pq);
    return true;
}

// 提供给全局任意使用
SEMAPHORE semaphore;