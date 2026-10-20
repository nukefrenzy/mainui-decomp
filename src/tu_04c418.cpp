// Circular singly linked lists used by the hardware test code.
// Proven 0x04c418-0x04cb68: 1,872 bytes, ten functions, no generated tail.
// The 0x04c418 and 0x04c7a8 starts are missing from the Ghidra export.
#include <cstdlib>
struct RingNode {
    void *value;
    RingNode *next;
};
RingNode *ringAt(RingNode *head,int index) {
    int i=0;
    if (head&&index>=0) {
        if (index==0)return head;
        i=1;
        RingNode*node=head->next;
        while (node!=head) {
            if (i==index)return node;
            i++;
            node=node->next;
        }
    }
    return NULL;
}

void freeRing(RingNode*head,void(*release)(void*)) {
    if (head) {
        RingNode*next=head;
        RingNode*node=head;
        do {
            next=node->next;
            if (release)release(node->value);
            free(node);
            node=next;
        }
        while (next!=head);
    }
}

RingNode *insertRingSorted(RingNode*head,void*value,int(*compare)(void*,void*),int unique) {
    RingNode*node=head;
    RingNode*previous=NULL;
    RingNode*result=head;
    RingNode*added=(RingNode*)malloc(sizeof(RingNode));
    added->value=value;
    if (head) {
        int count=0;
        node=head;
        while ((count==0||head!=node->next)&&compare(value,node->value)>0) {
            previous=node;
            node=node->next;
            count++;
        }
        if (unique&&compare(value,node->value)==0) {
            free(added);
            return result;
        }
        if (previous) {
            added->next=node;
            previous->next=added;
        } else {
            previous=head;
            while (head!=previous->next)previous=previous->next;
            added->next=node;
            previous->next=added;
            result=added;
        }
    } else {
        result=added;
        added->next=added;
    }
    return result;
}

RingNode *insertRing(RingNode*head,void*value,int(*compare)(void*,void*)) {
    return insertRingSorted(head,value,compare,0);
}

RingNode *insertRingUnique(RingNode*head,void*value,int(*compare)(void*,void*)) {
    return insertRingSorted(head,value,compare,1);
}

int ringCount(RingNode*head) {
    RingNode*node;
    int count=0;
    if (head) {
        count=1;
        node=head->next;
        while (node!=head) {
            count++;
            node=node->next;
        }
    }
    return count;
}

RingNode *appendRing(RingNode*head,void*value) {
    RingNode*node;
    int count;
    RingNode*added=(RingNode*)malloc(sizeof(RingNode));
    added->value=value;
    if (!head) {
        head=added;
        added->next=added;
    } else {
        count=0;
        node=head;
        while (head!=node->next||count==0) {
            node=node->next;
            count++;
        }
        node->next=added;
        added->next=head;
    }
    return head;
}

RingNode *prependRing(RingNode*head,void*value) {
    RingNode*node;
    int count;
    RingNode*added=(RingNode*)malloc(sizeof(RingNode));
    added->value=value;
    added->next=head;
    if (head) {
        count=0;
        node=head;
        while (head!=node->next||count==0) {
            node=node->next;
            count++;
        }
        node->next=added;
    } else added->next=added;
    return added;
}

RingNode *removeRingNode(RingNode*head,RingNode*remove) {
    if (!head)return NULL;
    int count=0;
    RingNode*result=head;
    RingNode*previous=NULL;
    RingNode*node=head;
    while (count==0||node!=head) {
        if (node==remove) {
            if (previous) {
                previous->next=node->next;
            } else {
                if (node==node->next)result=NULL;
                else {
                    result=node->next;
                    node=result;
                    while (head!=node->next)node=node->next;
                    node->next=result;
                }
            }
            free(remove);
            break;
        }
        previous=node;
        node=node->next;
        count++;
    }
    return result;
}

void forEachRing(RingNode*head,void(*visit)(void*)) {
    RingNode*node;
    int count;
    if (head) {
        count=0;
        node=head;
        while (node!=head||count==0) {
            visit(node->value);
            node=node->next;
            count++;
        }
    }
}
