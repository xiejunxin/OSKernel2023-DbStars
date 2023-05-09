#include <vmm.hpp>

VMS *VMS::KernelVMS = nullptr,
    *VMS::CurVMS = nullptr;

bool PAGETABLE::Init(bool isRoot)
{
    if (isRoot)
        memcpy(entries, (char *)boot_sv39_page_table, sizeof(ENTRY) * ENTRYCOUNT);
    else
        memset(entries, uint8(0), sizeof(ENTRY) * ENTRYCOUNT);
    return true;
}

bool PAGETABLE::Destroy()
{

    if (this == (void *)boot_sv39_page_table)
    {
        kout[red] << "can't Destroy Kernel PageTable" << endl;
        return false;
    }

    for (int i = 0; i < ENTRYCOUNT; i++)
    {
        if (entries[i].V && !entries[i].isKernelEntry())
        {
            if (entries[i].is_leaf())
            {
                pmm.free(entries[i].get_next_page());
            }
            else
                entries[i].get_next_page()->Destroy();
        }
        pmm.free(entries);
    }
    kout[green] << "success Destroy" << endl;
    return true;
}

void PAGETABLE::show()
{
    if (entries == nullptr)
    {
        return;
    }

    kout[blue] << "--------------" << endl;
    for (int i = 0; i < ENTRYCOUNT; i++)
    {
        if (entries[i].V)
        {
            if (entries[i].is_leaf())
            {
                kout[blue] << "~~~~~~~~~~" << endl;
                kout[purple] << i << endl;
                kout[blue] << KOUT::hex(entries[i].get_PNN()) << endl;
                kout[blue] << "~~~~~~~~~~" << endl;
            }
            else
            {
                entries[i].get_next_page()->show();
            }
        }
    }
    kout[blue] << "--------------" << endl;
}

// bool PAGETABLE::del(uint64 start, uint64 end, uint8 level)
// {
//     uint64 a, b, h, e;
//     uint64 mask = 0x1ff;
//     a = ((start >> (level * 9 + 12)) & mask);
//     b = ((end >> (level * 9 + 12)) & mask);
//     if (a == b)
//     {
//         if (entries[a].V && !entries[a].is_leaf())
//         {
//             entries[a].get_next_page()->del((start >> ((level - 1) * 9 + 12)) & mask,)
//         }
//     }
// }
bool PAGETABLE::del(uint64 start, uint64 end, uint8 level)
{
    return true;
}

bool VMR::Init(uint64 _start, uint64 _end, uint32 _flags)
{
    start = _start;
    end = _end;
    flag.flag = _flags;
    pre = nullptr;
    next = nullptr;
    return true;
}

bool VMS::Init()
{
    Head = nullptr;
    VMRCount = 0;

    PDT=(PAGETABLE *)pmm.malloc(4096);
    PDT ->Init(1);

    ShareCount = 0;
    return true;
}

void VMS::insert(VMR *tar)
{
    kout[green] << "success to insert" << endl;
    tar->pre = nullptr;
    tar->next = Head;
    if (Head != nullptr)
    {
        Head->pre = tar;
    }
    Head = tar;
}

void VMS::leave()
{
    KernelVMS->Enter();
}

void VMS::Enter()
{
    if (CurVMS == this)
        return;
    CurVMS = this;
    lcr3((uint64)PDT->PAddr());
    asm volatile("sfence.vma \n fence.i \n fence");
}

bool VMS::del(bool (*p)(VMR *tar))
{
    VMR *t, *t1;
    while (Head != nullptr && p(Head))
    {
        t = Head;
        pmm.free(t);
        Head = Head->next;
        VMRCount--;
        Head->pre = nullptr;
        PDT->del(t->start, t->end, 2);
    }

    if (Head == nullptr)
    {
        kout[green] << "success del VMR" << endl;
        return true;
    }

    while (t->next != nullptr)
    {
        while (t->next && p(t->next))
        {
            t1 = t->next;
            t->next = t1->next;
            if (t1->next)
                t1->next->pre = t;
            pmm.free(t1);
            VMRCount--;
            PDT->del(t1->start, t1->end, 2);
        }
        t = t->next;
    }
    kout[green] << "success del VMR" << endl;
    return true;
}
bool VMS::del(VMR *tar)
{
    if (tar->pre == nullptr && tar->next == nullptr)
    {
        Head = nullptr;
        return true;
    }
    if (tar->pre)
        tar->pre->next = tar->next;
    if (tar->next)
        tar->next->pre = tar->pre;
    return false;
}

VMR *VMS::find(void *addr)
{
    VMR *t=Head;
    while (t!=nullptr)
    {
        if(t->GetStart() <=(uint64)addr&&t->GetEnd()>(uint64)addr)
            return t;
        t=t->next;
    }
    return nullptr;
}

bool VMS::Static_Init()
{
    KernelVMS = (VMS *)pmm.malloc(sizeof(VMS));
    if (KernelVMS == nullptr)
    {
        kout[red] << "failed to malloc Kernel VMS" << endl;
        return false;
    }

    KernelVMS->PDT = (PAGETABLE *)boot_sv39_page_table;
    KernelVMS->VMRCount = 1;

    KernelVMS->Head = (VMR *)pmm.malloc(sizeof(VMR));
    KernelVMS->Head->Init(0xffffffff00000000, 0xffffffffc0000000, 39);

    KernelVMS->ShareCount = 1;
    return true;
}

bool VMS::clear()
{
    if (ShareCount > 1)
    {
        kout[red] << "failed to clear VMS.More than one process use this VMS" << endl;
        return false;
    }
    else
    {
        if (Head != nullptr)
        {
            VMR *t;
            while (Head->next)
            {
                t = Head->next;
                pmm.free(t);
                Head->next = t->next;
            }
            pmm.free(Head);
            Head = nullptr;
        }
        PDT->Destroy();
    }
    ShareCount = 1;

    kout[green] << "success clear VMS" << endl;
    return true;
}

bool VMS::destroy()
{
    if (!this->clear())
    {
        return false;
    }
    pmm.free(PDT);
    pmm.free(this);
    return true;
}

void VMS::show()
{
    VMR *t = Head;
    while (t)
    {
        kout[blue] << "start :" << KOUT::hex(t->GetStart()) << endl;
        kout[blue] << "end   :" << KOUT::hex(t->GetEnd()) << endl;
        kout[blue] << "flag  :" << KOUT::hex(t->GetFlags().flag) << endl;
        t = t->next;
    }
    kout << endl;
}

bool VMS::SolvePageFault(TRAPFRAME *tf)
{
    VMR * t=find((void*)(tf->badvaddr));
    if(t==nullptr)
    {
        kout[red]<<"Invalid addr"<<endl;
        return false;
    }
    ENTRY &e2=PDT->getEntry(((tf->badvaddr>>12)>>(9*2))&511);
    PAGETABLE *p;
    if (!e2.V)
    {
        p=(PAGETABLE *)pmm.malloc(4096); 
        p->Init(); 
        e2.V=1;
        e2.W=0;
        e2.R=0;
        e2.X=0;
    }
    else
        p=e2.get_next_page();
    

    ENTRY &e1=PDT->getEntry(((tf->badvaddr>>12)>>(9*1))&511);
    if (!e1.V)
    {
        p=(PAGETABLE *)pmm.malloc(4096); 
        p->Init(); 
        e1.V=1;
        e1.W=0;
        e1.R=0;
        e1.X=0;
    }
    else
        p=e1.get_next_page();
 
    ENTRY &e0=PDT->getEntry((tf->badvaddr>>12)&511);
    if (!e0.V)
    {
        p=(PAGETABLE *)pmm.malloc(4096); 
        p->Init(); 
        e0.V=1;
        e0.W=0;
        e0.R=0;
        e0.X=0;
    }
    else
    {
        kout[red]<<"page exeis but still error"<<endl;
        return false;
    }
 
    asm volatile("sfence.vma \n fence.i \n fence");
    return true;
}


bool TrapFunc_PageFault(TRAPFRAME *tf)
{
    return VMS::GetCurVMS()->SolvePageFault(tf);
}