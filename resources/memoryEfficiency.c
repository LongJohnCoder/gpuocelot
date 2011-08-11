{
    unsigned long threadId = blockThreadId();
    
    ON_INSTRUCTION:
    MEM_READ:
    MEM_WRITE:
    GLOBAL:
    {
        SHARED_MEM: 
        {
            sharedMem[threadId] = computeBaseAddress();
        }
        
        if(leastActiveThreadInWarp() != 0)
        {
            GLOBAL_MEM:
            {
                deviceMem[0] = deviceMem[0] + uniqueElementCount(sharedMem);
            }
        } 
    }    
}
