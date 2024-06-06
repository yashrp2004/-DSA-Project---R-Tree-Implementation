#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <limits.h>
#include <math.h>
#include <SDL2/SDL.h>
#define m 2
#define M 4
#define MAX_TYPE_LEN 50
#define SCREEN_WIDTH 1280
#define SCREEN_HEIGHT 720
#define NODE_RADIUS 20
#define MAX_OBJECTS 1000
#define K_NEAREST_NEIGHBORS 5
//******************************************************************************************************************************************************************
// Defining various struct



// Stores the 2-D object.
struct object
{
    int x;
    int y;
    char type[MAX_TYPE_LEN];
};
typedef struct object * OBJ;

// Stores the bounding rectangles of nodes.
struct rectangle
{
    int min_x;
    int min_y;
    int max_x;
    int max_y;
};
typedef struct rectangle * RECT;


// Stores the details of a node of R-Tree.
struct node
{

    bool is_leaf;                // Stores whether node is leaf or internal
    int count;                   // Stores the count of children or objects
    struct node * parent;        // Stores the parent of node
    RECT regions[M];             // Stores the bounding box of children or object
    struct node * children[M];   // Stores the children of node if it is a internal node
    OBJ objects[M];              // Stores the objects stored if it is a leaf node
    char name[50];               // New field for node name

};
typedef struct node * NODE;

// Stores the details of the R-Tree
struct r_tree
{
    int height;
    RECT rect;
    NODE root;
};
typedef struct r_tree * R_TREE;

// SDL variables
SDL_Window *window = NULL;
SDL_Renderer *renderer = NULL;
/*
// Priority queue node for storing objects and their distances
typedef struct {
    OBJ object;
    double distance;
} PriorityNode;

// Priority queue (min-heap) structure
typedef struct {
    PriorityNode* heap;
    int size;
    int capacity;
} PriorityQueue; */


//******************************************************************************************************************************************************************



//******************************************************************************************************************************************************************
// Function declaration
NODE create_new_leaf_node();
NODE create_new_internal_node();
R_TREE create_new_r_tree();
RECT create_new_rect(int min_x, int min_y, int max_x, int max_y);
OBJ create_new_object(int x, int y, const char* type_name);
void insert_object_into_node(NODE node, OBJ object, RECT rect);
void insert_region_into_node(NODE parent_node, NODE child_node, RECT region);
long long area_rect(RECT rect);
long long increase_in_area(RECT rect1, RECT rect2);
NODE choose_leaf(NODE node, OBJ object);
RECT bounding_box(NODE node);
int * pick_seeds(NODE node, RECT rect);
bool search_in_node(NODE node, RECT rect);
int pick_next(NODE node1, NODE node2, NODE node, RECT rect);
NODE * quadratic_split_leaf_node(NODE node, OBJ object);
void adjust_tree(R_TREE r_tree, NODE node1, NODE node2, NODE node);
NODE * quadratic_split_internal_node(NODE node, RECT rect, NODE child);
void insert_in_r_tree(R_TREE r_tree, OBJ object);
void pre_order_traversal(NODE node, int depth);
double euclidean_distance(int x1, int y1, int x2, int y2);
bool rect_intersects(RECT rect1, RECT rect2);
void search_in_r_tree(NODE node, RECT rect, int user_x, int user_y, double radius, OBJ found_objects[], int* num_found);
OBJ search_nearest_neighbor(NODE node, RECT rect, int user_x, int user_y, OBJ nearest_neighbor, double* min_distance);
double euclidean_distance(int x1, int y1, int x2, int y2);
int assign_internal_node_names(struct node *node, int region_counter);
void find_k_nearest_neighbors(NODE root, int user_x, int user_y, int K, OBJ* neighbors);

//******************************************************************************************************************************************************************




//******************************************************************************************************************************************************************
// General Helper Functions

// Creates new leaf node
NODE create_new_leaf_node()
{
    NODE new_leaf = (NODE) malloc(sizeof(struct node));
    new_leaf -> is_leaf = true;
    new_leaf -> count = 0;
    new_leaf -> parent = NULL;
    for(int i = 0; i < M; ++i)
    {
        (new_leaf -> regions)[i] = NULL;
        (new_leaf -> children)[i] = NULL;
        (new_leaf -> objects)[i]  = NULL;
    }
    return new_leaf;
}

// Creates new internal node
NODE create_new_internal_node()
{
    NODE new_internal = (NODE) malloc(sizeof(struct node));
    new_internal -> is_leaf = false;
    new_internal -> count = 0;
    new_internal -> parent = NULL;
    for(int i = 0; i < M; ++i)
    {
        (new_internal -> regions)[i] = NULL;
        (new_internal -> children)[i] = NULL;
        (new_internal -> objects)[i]  = NULL;
    }
    return new_internal;
}

// Creates new R-Tree.
R_TREE create_new_r_tree()
{
    R_TREE new_r_tree = (R_TREE) malloc(sizeof(struct r_tree));
    new_r_tree -> height = 0;
    new_r_tree -> rect =NULL;
    new_r_tree -> root = create_new_leaf_node();
    return new_r_tree;
}

// Creates new bounding rectangle
RECT create_new_rect(int min_x, int min_y, int max_x, int max_y )
{
    RECT new_rect = (RECT) malloc(sizeof(struct rectangle));
    new_rect -> min_x = min_x;
    new_rect -> min_y = min_y;
    new_rect -> max_x = max_x;
    new_rect -> max_y = max_y;
    return new_rect;
}

// Creates new object
OBJ create_new_object(int x, int y, const char* type_name)
{
    OBJ new_object = (OBJ) malloc(sizeof(struct object));
    new_object->x = x;
    new_object->y = y;
    strcpy(new_object->type, type_name); // Set the type name for the object
    return new_object;
}

// Inserts object and its bounding rectangle in leaf node.
void insert_object_into_node(NODE node, OBJ object, RECT rect)
{
    int i = 0;
    while((node -> objects)[i] != NULL)
        ++i;
    (node -> objects)[i] = object;
    (node -> regions)[i] = rect;
    node -> count += 1;
}

// Inserts bounding rectangle and the regions contained in it as children in internal node
void insert_region_into_node(NODE parent_node, NODE child_node, RECT region)
{
    int i = 0;
    while((parent_node -> children)[i] != NULL)
        ++i;
    (parent_node -> regions)[i] = region;
    (parent_node -> children)[i] = child_node;
    parent_node -> count += 1;
    child_node -> parent = parent_node;
}

// Calculates the area of the bounding rectangle.
long long area_rect(RECT rect)
{
    return (long long)(rect -> max_x - rect -> min_x) * (long long)(rect -> max_y - rect -> min_y);
}

// Calculates the area enlargement i.e. increase in area of rect1 required to contain rect2 within itself
long long increase_in_area(RECT rect1, RECT rect2)
{
    int min_x =  rect1 -> min_x <= rect2 -> min_x ? rect1 -> min_x : rect2 -> min_x;
    int min_y =  rect1 -> min_y <= rect2 -> min_y ? rect1 -> min_y : rect2 -> min_y;
    int max_x =  rect1 -> max_x >= rect2 -> max_x ? rect1 -> max_x : rect2 -> max_x;
    int max_y =  rect1 -> max_y >= rect2 -> max_y ? rect1 -> max_y : rect2 -> max_y;
    return (long long)(max_x - min_x) * (long long)(max_y - min_y) -  area_rect(rect1);

}

// Creates the bounding box of a paricular node.
RECT bounding_box(NODE node)
{
    int i = 0;
    int min_x = INT_MAX;
    int min_y = INT_MAX;
    int max_x = INT_MIN;
    int max_y = INT_MIN;
    while(i < M && (node -> regions)[i] != NULL)
    {
        min_x = min_x <= (node -> regions)[i] -> min_x ? min_x : (node -> regions)[i] -> min_x;
        min_y = min_y <= (node -> regions)[i] -> min_y ? min_y : (node -> regions)[i] -> min_y;
        max_x = max_x >= (node -> regions)[i] -> max_x ? max_x : (node -> regions)[i] -> max_x;
        max_y = max_y >= (node -> regions)[i] -> max_y ? max_y : (node -> regions)[i] -> max_y;
        ++i;
    }
    return create_new_rect(min_x,min_y,max_x,max_y);
}

// Checks whether a children is present in a node or not through bounding rectangle
bool search_in_node(NODE node, RECT rect)
{
    int i = 0;
    while(i < M && (node -> regions)[i] != NULL)
    {
        if((node -> regions)[i] == rect)
            return true;
        ++i;
    }
    return false;
}

//******************************************************************************************************************************************************************






//******************************************************************************************************************************************************************
// Hepler functions for Insertions

// Selects a leaf node to place a new entry i.e. object.
NODE choose_leaf(NODE node, OBJ object)
{
    //CL2: If node  is leaf, return the node.
    if(node -> is_leaf)
        return node;

    // CL3: If the node is not a leaf node, select the subtree which requires minimum enlargement.
    long long  min_enlargement = LLONG_MAX;
    RECT obj_rect = create_new_rect(object -> x, object -> y, object -> x, object -> y);
    // Index stores which subtree will get choosen finally.
    int index = -1;
    // Iterating over all subtrees of a node
    for(int i = 0; i < node -> count; ++i )
    {
        // If ith index requires lesser enlargement, choose it
        if(increase_in_area((node -> regions)[i], obj_rect) < min_enlargement)
        {
            min_enlargement = increase_in_area((node -> regions)[i], obj_rect);
            index = i;
        }
        // If ith index requires same enlargement choose the one with minimum area
        else if(increase_in_area((node -> regions)[i], obj_rect) == min_enlargement)
        {
            index = area_rect((node -> regions)[index]) <= area_rect((node -> regions)[i]) ? index : i;
        }
    }
    free(obj_rect);

    //CL4: Descend until a leaf node is choosen
    return choose_leaf((node -> children)[index], object);
}

// Splits the leaf node into two nodes
NODE * quadratic_split_leaf_node(NODE node, OBJ object)
{
    // Creating the two leaf nodes after split
    NODE node1 = create_new_leaf_node();
    NODE node2 = create_new_leaf_node();

    RECT obj_rect = create_new_rect(object -> x, object -> y, object -> x, object -> y);

    // QS1: Call pick_seed function to get first entries of splitted nodes and insert those enteries in the nodes
    int * pair = pick_seeds(node, obj_rect);
    insert_object_into_node(node1, (node -> objects)[pair[0]], (node -> regions)[pair[0]]);
    if(pair[1] == M)
        insert_object_into_node(node2, object, obj_rect);
    else
        insert_object_into_node(node2, (node -> objects)[pair[1]],(node -> regions)[pair[1]]);

    int index = -1;

    //QS2.1: Check if all the entries of node and object is assigned to either node1 or node2. If done, come out of loop.
    while(pick_next(node1,node2,node,obj_rect) != -1)
    {
        index = -1;

        //QS2.2: If a group requires that remaining entries must be assigned to it for it to have the minimum number m, then that assign entries to group one by one
        NODE final_node;
        if(M + 1 - node2 -> count == m)
            final_node = node1;
        else if(M + 1 - node1 -> count == m)
            final_node = node2;
        // If this is not the case as mentioned in QS2.2
        else
        {
            //QS3.1: Call pick_next function to select that next entry to be inserted
            index = pick_next(node1,node2,node,obj_rect);

            long long d1,d2;
            RECT bound_node1 = bounding_box(node1);
            RECT bound_node2 = bounding_box(node2);

            //QS3.2: Calculate the enlargment i.e. increase in area required by both groups
            if(index != M)
            {
                d1 = increase_in_area(bound_node1, (node -> regions)[index]);
                d2 = increase_in_area(bound_node2, (node -> regions)[index]);
            }
            else
            {
                d1 = increase_in_area(bound_node1, obj_rect);
                d2 = increase_in_area(bound_node2, obj_rect);
            }

            //QS3.3: Select the group that requires lesser enlargement
            if(d1 != d2)
                final_node = d1 < d2 ? node1 : node2;
            // In case they require same enlargement
            else
            {
                long long area1 = area_rect(bound_node1);
                long long area2 = area_rect(bound_node2);

                //QS3.4: Choose the group with lesser area
                if(area1 != area2)
                    final_node = area1 < area2 ? node1 : node2;
                // In case they have same area
                else
                    //QS3.5: Choose the group with lesser children
                    final_node = node1 -> count <= node2 -> count ? node1 : node2;
            }
            free(bound_node1);
            free(bound_node2);
        }
        if(index == -1)
            index = pick_next(node1,node2,node,obj_rect);

        // Insert the entries into respective nodes.
        if(index != M)
            insert_object_into_node(final_node, (node -> objects)[index], (node -> regions)[index]);
        else
            insert_object_into_node(final_node, object, obj_rect);
    }
    NODE * splitted_nodes = (NODE *)malloc(sizeof(NODE) * 2);
    splitted_nodes[0] = node1;
    splitted_nodes[1] = node2;
    return splitted_nodes;
}


NODE * quadratic_split_internal_node(NODE node, RECT rect, NODE child)
{
    // Creating the two internal nodes after split
    NODE node1 = create_new_internal_node();
    NODE node2 = create_new_internal_node();

    // QS1: Call pick_seed function to get first entries of splitted nodes and insert those enteries in the nodes
    int * pair = pick_seeds(node, rect);
    insert_region_into_node(node1, (node -> children)[pair[0]], (node -> regions)[pair[0]]);
    if(pair[1] == M)
        insert_region_into_node(node2, child, rect);
    else
        insert_region_into_node(node2, (node -> children)[pair[1]],(node -> regions)[pair[1]]);

    int index = -1;

    //QS2.1: Check if all the entries of node and object is assigned to either node1 or node2. If done, come out of loop.
    while(pick_next(node1,node2,node, rect) != -1)
    {
        index = -1;

        //QS2.2: If a group requires that remaining entries must be assigned to it for it to have the minimum number m, then that assign entries to group one by one
        NODE final_node;
        if(M + 1 - node2 -> count == m)
            final_node = node1;
        else if(M + 1 - node1 -> count == m)
            final_node = node2;
        // If this is not the case as mentioned in QS2.2
        else
        {
            //QS3.1: Call pick_next function to select that next entry to be inserted
            index = pick_next(node1,node2,node,rect);

            long long d1,d2;
            RECT bound_node1 = bounding_box(node1);
            RECT bound_node2 = bounding_box(node2);

            //QS3.2: Calculate the enlargment i.e. increase in area required by both groups
            if(index != M)
            {
                d1 = increase_in_area(bound_node1,(node -> regions)[index]);
                d2 = increase_in_area(bound_node2,(node -> regions)[index]);
            }
            else
            {
                d1 = increase_in_area(bound_node1, rect);
                d2 = increase_in_area(bound_node2, rect);
            }

            //QS3.3: Select the group that requires lesser enlargement
            if(d1 != d2)
                final_node = d1 < d2 ? node1 : node2;
            // In case they require same enlargement
            else
            {
                long long area1 = area_rect(bound_node1);
                long long area2 = area_rect(bound_node2);
                //QS3.4: Choose the group with lesser area
                if(area1 != area2)
                    final_node = area1 < area2 ? node1 : node2;
                // In case they have same area
                else
                //QS3.5: Choose the group with lesser children
                    final_node = node1 -> count <= node2 -> count ? node1 : node2;
            }
            free(bound_node1);
            free(bound_node2);
        }
        if(index == -1)
            index = pick_next(node1,node2,node,rect);

        // Insert the entries into respective nodes.
        if(index != M)
            insert_region_into_node(final_node, (node -> children)[index], (node -> regions)[index]);
        else
            insert_region_into_node(final_node, child, rect);

    }
    NODE * splitted_nodes = (NODE *)malloc(sizeof(NODE) * 2);
    splitted_nodes[0] = node1;
    splitted_nodes[1] = node2;
    return splitted_nodes;
}

// Propagates changes made to leaf upwards in the tree by updating MBR and splits if required
void adjust_tree(R_TREE r_tree, NODE node1, NODE node2, NODE node)
{
    // AT2: if node is root stop the upward traversal after making changes
    if(node == r_tree -> root)
        // If root node does not require to be splitted.
        if(node1 == NULL && node2 == NULL)
        {
            free(r_tree -> rect);

            // Update the bounding rectangle of root
            r_tree -> rect = bounding_box(node);
        }
        // If root node requires to be splitted
        else
        {
            // Create a new root node
            NODE new_root = create_new_internal_node();
            // Insert the splitted root nodes as children of new root
            insert_region_into_node(new_root, node1, bounding_box(node1));
            insert_region_into_node(new_root, node2, bounding_box(node2));

            r_tree -> height  = r_tree -> height + 1;
            r_tree -> root = new_root;
            free(r_tree -> rect);
            r_tree -> rect = bounding_box(new_root);
            free(node);
        }
    else
    {
        NODE parent = node -> parent;
        int i = 0;
        while(i < M && (parent -> children)[i] != NULL)
        {
            if((parent -> children)[i] == node)
                break;
            ++i;
        }

        // If  node does not require to be splitted
        if(node1 == NULL && node2 == NULL)
        {
            free((parent -> regions)[i]);
            //AT3: Adjust the bounding box of node in its parent
            (parent -> regions)[i] = bounding_box(node);

            //AT5: Propagate the change upwards
            adjust_tree(r_tree, NULL, NULL, parent);
        }
        // If node  need to be splitted
        else
        {
            (parent -> children)[i] = node1;
            node1 -> parent = parent;
            free(node);
            free((parent -> regions)[i]);
            (parent -> regions)[i] = bounding_box(node1);

            // If the parent need to be splitted
            if(parent -> count == M)
            {
                //AT4.1: Call split node function to get splitted nodes
                NODE * nodes = quadratic_split_internal_node(parent, bounding_box(node2), node2);

                //AT5: Propagate the change upwards
                adjust_tree(r_tree, nodes[0],nodes[1], parent);
            }
            // If the parent does not need to be splitted
            else
            {
                //AT4: insert the node into parent.
                insert_region_into_node(parent,node2,bounding_box(node2));

                //AT5: Propagate the change upwards
                adjust_tree(r_tree, NULL, NULL, parent);
            }
        }
    }

}

//******************************************************************************************************************************************************************









//******************************************************************************************************************************************************************
// Helper functions for Split Node

// Choose two entries to be the first elements of the two group after split.
int * pick_seeds(NODE node, RECT rect)
{
    long long max_d = LLONG_MIN;

    // Index1 and Index2 stores the indexes of two corresponding entries
    int index1 = -1;
    int index2 = -1;

    // PS1: Calculating the inefficiency of grouping two entries togther
    for(int i = 0; i < M; ++i)
    {
        long long d = increase_in_area((node -> regions)[i], rect) - area_rect(rect);

        // PS2: Choose the pair having maximum inefficiency
        if(d > max_d)
        {
            max_d = d;
            index1 = i;
            index2 = M;
        }
    }
    for(int i = 0; i < M; ++i)
    {
        for(int j = i + 1; j < M; ++j)
        {
            long long d = increase_in_area((node -> regions)[i], (node -> regions)[j]) - area_rect((node -> regions)[j]);

            // PS2: Choose the pair having maximum inefficiency.
            if(d > max_d)
            {
                max_d = d;
                index1 = i;
                index2 = j;
            }
        }
    }
    int* pair = (int *)malloc(sizeof(int) * 2);
    pair[0] = index1;
    pair[1] = index2;
    return pair;
}

//Choose one remaining entry for classification in group after split.
int pick_next(NODE node1, NODE node2, NODE node, RECT rect)
{
    // Indexes stores the indices of the entries that are yet to be put in one of the groups
    int * indexes = (int *) malloc(sizeof(int) * (M - 1));

    // Checking which of M + 1 entries are yet to be classified using search_in_node
    int count = 0;
    for(int i = 0; i < M; ++i)
    {
        if(!search_in_node(node1,(node -> regions)[i]) && !search_in_node(node2, (node -> regions)[i]))
            indexes[count++] = i;
    }
    if(!search_in_node(node1,rect) && !search_in_node(node2, rect))
            indexes[count++] = M;
    if(count == 0)
        return -1;

    long long max_d = LLONG_MIN;

    // final_index stores the index of the next entry that needs to be classified
    int final_index = -1;
    RECT bound_node1 = bounding_box(node1);
    RECT bound_node2 = bounding_box(node2);
    for(int i = 0; i < count; ++i)
    {
        if(indexes[i] == M)
        {
            // PN1: Calculate the cost of putting each entry in each group and take difference
            long long d = increase_in_area(bound_node1,rect) - increase_in_area(bound_node2,rect);
            d = d >= 0 ? d : -d;

            //PN2: Choose the entry with maximum difference
            if(max_d < d)
            {
                max_d = d;
                final_index = M;

            }
        }
        else
        {
            // PN1: Calculate the cost of putting each entry in each group and take difference
            long long d = increase_in_area(bound_node1,(node -> regions)[indexes[i]]) - increase_in_area(bound_node2,(node -> regions)[indexes[i]]);
            d = d >= 0 ? d : -d;

            //PN2: Choose the entry with maximum difference
            if(max_d < d)
            {
                max_d = d;
                final_index = indexes[i];
            }
        }

    }
    free(bound_node1);
    free(bound_node2);
    return final_index;
}

//******************************************************************************************************************************************************************





//******************************************************************************************************************************************************************

// Inserts a new object in the R-Tree
void insert_in_r_tree(R_TREE r_tree, OBJ object)
{
    // I1: Call the choose_leaf function to get the leaf node where object needs to be placed
    NODE node = choose_leaf(r_tree -> root, object);

    // If leaf node is already full
    if(node -> count == M)
    {
        // I2.1: Call split node function to get the splitted nodes
        NODE * nodes = quadratic_split_leaf_node(node,object);

        //I3: Propagate the change upwards in the tree.
        adjust_tree(r_tree, nodes[0],nodes[1],node);

    }
    // If leaf node is not full
    else
    {
        // I2.2: Insert the object into the leaf node
        insert_object_into_node(node, object, create_new_rect(object -> x, object -> y, object -> x, object -> y));

        //I3: Propagate the change upwards in the tree.
        adjust_tree(r_tree, NULL, NULL, node);
    }
}

//******************************************************************************************************************************************************************




//******************************************************************************************************************************************************************
void pre_order_traversal(NODE node, int depth)
{
    // If node is null, then return
    if (node == NULL)
        return;

    for(int i =0; i < depth; ++i )
        printf("  ");


    //  If the node is a leaf node
    if (node -> is_leaf)
    {
        printf("Leaf Node: ");
        int i =0;

        // Print the objects contained within the leaf node
        while(i < M && (node -> regions)[i] != NULL)
        {
            printf("[(%d, %d) - %s]", node -> objects[i] -> x, node -> objects[i] -> y , node -> objects[i] -> type);
            if (i < node -> count - 1)
                printf(", ");
            ++i;
        }
        printf("\n");
    }
    // If the node is internal node
    else
    {
        printf("Internal Node (%s): ", node->name);
        RECT rect = bounding_box(node);

        // Print the bounding box of the internal node
        printf("[(%d, %d), (%d, %d)]", rect -> min_x, rect -> min_y, rect -> max_x, rect -> max_y);
        printf("\n");
        free(rect);
        /*
        //To print bounding boxes of the children uncomment this section
        int i = 0;
        while(i < M && (node -> regions)[i] != NULL)
        {
            printf("[(%d, %d), (%d, %d)]", node->regions[i]->min_x, node->regions[i]->min_y, node->regions[i]->max_x, node->regions[i]->max_y);
            if (i < node -> count - 1)
                printf(", ");
            ++i;
        }
        printf("\n");
        */

        // Recursive Call the function to print the subtree of the node one by one
        int i = 0;
        while(i < M && (node -> regions)[i] != NULL)
        {
            pre_order_traversal(node -> children[i], depth + 1);
            ++i;
        }

    }
}




// Search for objects within a specified bounding rectangle
void search_in_r_tree(NODE node, RECT rect, int user_x, int user_y, double radius, OBJ found_objects[], int* num_found) {
    // If the node is null, return
    if (node == NULL)
        return;

    // If the node is a leaf node
    if (node->is_leaf) {
        // Iterate through the objects in the leaf node
        for (int i = 0; i < M && (node->objects)[i] != NULL; ++i) {
            int obj_x = (node->objects)[i]->x;
            int obj_y = (node->objects)[i]->y;
            double distance = euclidean_distance(user_x, user_y, obj_x, obj_y);
            // Check if the object is within the specified radius
            if (distance <= radius) {
                printf("Object at (%d, %d) is within the radius (%.2f) from the user.\n", obj_x, obj_y, radius);
                found_objects[(*num_found)++] = (node->objects)[i]; // Add the found object to the array
            }
        }
    } else {
        // If the node is an internal node
        // Check if the bounding rectangle of the node intersects with the specified rectangle
        if (rect_intersects(rect, bounding_box(node))) {
            // Recursively search the child nodes
            for (int i = 0; i < M && (node->children)[i] != NULL; ++i) {
                search_in_r_tree((node->children)[i], rect, user_x, user_y, radius, found_objects, num_found);
            }
        }
    }
}

// Search for the nearest neighbor
OBJ search_nearest_neighbor(NODE node, RECT rect, int user_x, int user_y, OBJ nearest_neighbor, double* min_distance) {
    // If the node is null, return
    if (node == NULL)
        return nearest_neighbor;

    // If the node is a leaf node
    if (node->is_leaf) {
        // Iterate through the objects in the leaf node
        for (int i = 0; i < M && (node->objects)[i] != NULL; ++i) {
            int obj_x = (node->objects)[i]->x;
            int obj_y = (node->objects)[i]->y;
            double distance = euclidean_distance(user_x, user_y, obj_x, obj_y);
            // Check if the object is closer than the current nearest neighbor
            if (distance < *min_distance) {
                *min_distance = distance;
                nearest_neighbor = (node->objects)[i];
            }
        }
    } else {
        // If the node is an internal node
        // Check if the bounding rectangle of the node intersects with the specified rectangle
        if (rect_intersects(rect, bounding_box(node))) {
            // Recursively search the child nodes
            for (int i = 0; i < M && (node->children)[i] != NULL; ++i) {
                nearest_neighbor = search_nearest_neighbor((node->children)[i], rect, user_x, user_y, nearest_neighbor, min_distance);
            }
        }
    }
    return nearest_neighbor;
}


// Check if two rectangles intersect
bool rect_intersects(RECT rect1, RECT rect2) {
    return !(rect2->min_x > rect1->max_x || rect2->max_x < rect1->min_x ||
             rect2->min_y > rect1->max_y || rect2->max_y < rect1->min_y);
}

double euclidean_distance(int x1, int y1, int x2, int y2) {
    return sqrt(pow(x2 - x1, 2) + pow(y2 - y1, 2));
}

int assign_internal_node_names(struct node *node, int region_counter) {
    if (node == NULL || node->is_leaf) {
        return region_counter;
    }

    // Assign name to the current node
    sprintf(node->name, "Region %d", region_counter);

    // Update region counter for the next internal node
    region_counter++;

    // Recursively assign names to children
    for (int i = 0; i < node->count; ++i) {
        region_counter = assign_internal_node_names(node->children[i], region_counter);
    }

    return region_counter;
}


// SDL Helper Functions

// Initialize SDL
bool init_sdl()
{
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
        return false;
    }
    window = SDL_CreateWindow("R-Tree Visualization", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    if (window == NULL)
    {
        printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
        return false;
    }
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (renderer == NULL)
    {
        printf("Renderer could not be created! SDL_Error: %s\n", SDL_GetError());
        return false;
    }
    return true;
}


// Renders the R-Tree node
// Renders the R-Tree node with minimum bounding rectangles around points
void render_r_tree_node(NODE node, int x, int y, int width, int height) {
    SDL_Rect rect = {x, y, width, height};
    SDL_SetRenderDrawColor(renderer, 0xFF, 0x00, 0x00, 0xFF);
    SDL_RenderDrawRect(renderer, &rect);
    if (!node->is_leaf) {
        for (int i = 0; i < node->count; ++i) {
            RECT region = node->regions[i];
            render_r_tree_node(node->children[i], region->min_x - 5, region->min_y - 5, region->max_x - region->min_x + 10, region->max_y - region->min_y + 10);
        }
    } else {
        for (int i = 0; i < node->count; ++i) {
            OBJ object = node->objects[i];
            // Draw bounding rectangle around point
            int rectSize = 5; // Size of bounding rectangle
            SDL_Rect pointRect = {object->x - rectSize/2, object->y - rectSize/2, rectSize, rectSize};
            SDL_SetRenderDrawColor(renderer, 0x00, 0xFF, 0xFF, 0xFF);
            SDL_RenderDrawRect(renderer, &pointRect);
            SDL_RenderDrawPoint(renderer, object->x, object->y);
        }
    }
}

// Renders yellow boxes around points within the specified radius
void render_points_within_radius(int user_x , int user_y , OBJ found_objects[], int num_found, double radius) {
    SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0x00, 0xFF); // Yellow color
    int rectSize = 10; // Size of the yellow box
    for (int i = 0; i < num_found; ++i) {
        OBJ object = found_objects[i];
        SDL_Rect rect = {object->x - rectSize/2, object->y - rectSize/2, rectSize, rectSize};
        SDL_RenderDrawRect(renderer, &rect);
        SDL_RenderDrawLine(renderer, user_x, user_y, object->x, object->y);
        SDL_RenderPresent(renderer);
    }
    SDL_RenderPresent(renderer);
}


// Renders the R-Tree
void render_r_tree(R_TREE r_tree)
{
    SDL_RenderClear(renderer);
    render_r_tree_node(r_tree->root, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
    SDL_RenderPresent(renderer);
}

//******************************************************************************************************************************************************************
// Initialize the priority queue
/*PriorityQueue* init_priority_queue(int capacity) {
    PriorityQueue* pq = (PriorityQueue*)malloc(sizeof(PriorityQueue));
    pq->heap = (PriorityNode*)malloc(capacity * sizeof(PriorityNode));
    pq->size = 0;
    pq->capacity = capacity;
    return pq;
}

// Insert a node into the priority queue
void insert_into_priority_queue(PriorityQueue* pq, OBJ object, double distance) {
    // Check if the object is already in the priority queue
    for (int i = 0; i < pq->size; ++i) {
        if (pq->heap[i].object->x == object->x &&
            pq->heap[i].object->y == object->y &&
            strcmp(pq->heap[i].object->type, object->type) == 0) {
            // If the object is already present, skip the insertion
            return;
        }
    }

    // If the object is not already present, insert it into the priority queue
    if (pq->size == pq->capacity)
        return; // Queue is full

    // Create a new priority node
    PriorityNode newNode;
    newNode.object = object;
    newNode.distance = distance;

    // Add the node to the end of the queue
    pq->heap[pq->size++] = newNode;

    // Perform heapify up to maintain heap property
    int i = pq->size - 1;
    while (i > 0 && pq->heap[(i - 1) / 2].distance > pq->heap[i].distance) {
        // Swap parent and child
        PriorityNode temp = pq->heap[(i - 1) / 2];
        pq->heap[(i - 1) / 2] = pq->heap[i];
        pq->heap[i] = temp;
        // Move up to parent
        i = (i - 1) / 2;
    }
}


// Extract the minimum node from the priority queue
PriorityNode extract_min_from_priority_queue(PriorityQueue* pq) {
    PriorityNode minNode = pq->heap[0];
    // Replace the root with the last node
    pq->heap[0] = pq->heap[pq->size - 1];
    pq->size--;
    // Perform heapify down to maintain heap property
    int i = 0;
    while (true) {
        int leftChild = 2 * i + 1;
        int rightChild = 2 * i + 2;
        int smallest = i;
        if (leftChild < pq->size && pq->heap[leftChild].distance < pq->heap[smallest].distance)
            smallest = leftChild;
        if (rightChild < pq->size && pq->heap[rightChild].distance < pq->heap[smallest].distance)
            smallest = rightChild;
        if (smallest != i) {
            // Swap parent and smallest child
            PriorityNode temp = pq->heap[i];
            pq->heap[i] = pq->heap[smallest];
            pq->heap[smallest] = temp;
            i = smallest; // Update the index
        } else {
            break;
        }
    }
    return minNode;
}


// Free the memory allocated for the priority queue
void free_priority_queue(PriorityQueue* pq) {
    free(pq->heap);
    free(pq);
}

void search_k_nearest_neighbors(NODE node, RECT rect, int user_x, int user_y, PriorityQueue* pq) {
    // If the node is null, return
    if (node == NULL)
        return;

    // If the node is a leaf node
    if (node->is_leaf) {
        // Iterate through the objects in the leaf node
        for (int i = 0; i < M && (node->objects)[i] != NULL; ++i) {
            int obj_x = (node->objects)[i]->x;
            int obj_y = (node->objects)[i]->y;
            double distance = euclidean_distance(user_x, user_y, obj_x, obj_y);

            // Check if the priority queue is not full or the distance is less than the maximum distance in the queue
            if (pq->size < K_NEAREST_NEIGHBORS || distance < pq->heap[0].distance) {
                // Check if the object is already in the priority queue
                bool found = false;
                for (int j = 0; j < pq->size; ++j) {
                    if (pq->heap[j].object == (node->objects)[i]) {
                        found = true;
                        break;
                    }
                }
                // If the object is not already present, insert it into the priority queue
                if (!found) {
                    insert_into_priority_queue(pq, (node->objects)[i], distance);
                    // If the priority queue size exceeds K_NEAREST_NEIGHBORS, remove the farthest object
                    if (pq->size > K_NEAREST_NEIGHBORS) {
                        extract_min_from_priority_queue(pq);
                    }
                }
            }
        }
    } else {
        // If the node is an internal node
        // Calculate the minimum distance between the user and the bounding box of the node
        RECT node_rect = bounding_box(node);
        double min_dist = INFINITY;
        double max_dist = 0.0;
        for (int i = 0; i < node->count; ++i) {
            RECT child_rect = node->regions[i];
            double dist_to_rect = euclidean_distance(user_x, user_y, (child_rect->min_x + child_rect->max_x) / 2, (child_rect->min_y + child_rect->max_y) / 2);
            min_dist = fmin(min_dist, dist_to_rect);
            max_dist = fmax(max_dist, dist_to_rect);
        }

        // If the minimum distance is greater than the maximum distance in the priority queue, we can prune this node
        if (min_dist > pq->heap[0].distance) {
            free(node_rect);
            return;
        }

        // If the maximum distance is less than or equal to the minimum distance in the priority queue, we can insert all objects in this node
        if (max_dist <= pq->heap[0].distance) {
            for (int i = 0; i < node->count; ++i) {
                NODE child_node = node->children[i];
                if (child_node->is_leaf) {
                    for (int j = 0; j < child_node->count; ++j) {
                        OBJ obj = child_node->objects[j];
                        double dist = euclidean_distance(user_x, user_y, obj->x, obj->y);
                        insert_into_priority_queue(pq, obj, dist);
                        if (pq->size > K_NEAREST_NEIGHBORS) {
                            extract_min_from_priority_queue(pq);
                        }
                    }
                }
            }
            free(node_rect);
            return;
        }

        // Otherwise, recursively search the child nodes
        if (rect_intersects(rect, node_rect)) {
            for (int i = 0; i < M && (node->children)[i] != NULL; ++i) {
                search_k_nearest_neighbors((node->children)[i], (node->regions)[i], user_x, user_y, pq);
            }
        }
        free(node_rect);
    }
}*/
//********************************************************************************************************************************************************

OBJ find_nearest_neighbor(NODE root, int user_x, int user_y) {
    OBJ nearest_neighbor = NULL;
    double min_distance = INFINITY;

    // Traverse the R-Tree and find the nearest neighbor
    nearest_neighbor = search_nearest_neighbor(root, bounding_box(root), user_x, user_y, nearest_neighbor, &min_distance);

    return nearest_neighbor;

}

OBJ search_next_nearest_neighbor(NODE root, RECT rect, int user_x, int user_y, OBJ* neighbors, int num_neighbors, double* min_distance) {
    OBJ next_nearest_neighbor = NULL;
    double current_min_distance = INFINITY;

    if (root == NULL)
        return NULL;

    if (root->is_leaf) {
        // If the current node is a leaf node, process its objects
        for (int i = 0; i < root->count; ++i) {
            OBJ obj = root->objects[i];
            double distance = euclidean_distance(user_x, user_y, obj->x, obj->y);

            // Check if the object is not already in the neighbors array and if it's closer than the current minimum distance
            bool found = false;
            for (int j = 0; j < num_neighbors; ++j) {
                if (neighbors[j] == obj) {
                    found = true;
                    break;
                }
            }

            if (!found && distance < current_min_distance) {
                current_min_distance = distance;
                next_nearest_neighbor = obj;
            }
        }
    } else {
        // If the current node is an internal node, process its children
        if (rect_intersects(rect, bounding_box(root))) {
            for (int i = 0; i < root->count; ++i) {
                OBJ next_neighbor = search_next_nearest_neighbor(root->children[i], root->regions[i], user_x, user_y, neighbors, num_neighbors, &current_min_distance);
                if (next_neighbor != NULL && current_min_distance < *min_distance) {
                    next_nearest_neighbor = next_neighbor;
                    *min_distance = current_min_distance; // Update the minimum distance
                }
            }
        }
    }

    // Update the minimum distance if it's changed
    if (current_min_distance < *min_distance) {
        *min_distance = current_min_distance;
    }

    return next_nearest_neighbor;
}


void find_k_nearest_neighbors(NODE root, int user_x, int user_y, int K, OBJ* neighbors) {
    OBJ nearest_neighbor;
    double min_distance;

    // Find the first nearest neighbor
    nearest_neighbor = find_nearest_neighbor(root, user_x, user_y);
    neighbors[0] = nearest_neighbor;
    min_distance = euclidean_distance(user_x, user_y, nearest_neighbor->x, nearest_neighbor->y);

    // Find the remaining K-1 nearest neighbors
    for (int i = 1; i < K; ++i) {
        nearest_neighbor = NULL;
        double current_min_distance = INFINITY;

        // Traverse the R-Tree and find the next nearest neighbor that is not already in the neighbors array
        nearest_neighbor = search_next_nearest_neighbor(root, bounding_box(root), user_x, user_y, neighbors, i, &current_min_distance);

        // Store the found neighbor
        neighbors[i] = nearest_neighbor;
        min_distance = current_min_distance;
    }
}








int main(int argc, char *argv[])  {



    // Initialize SDL
    if (!init_sdl()) {
        fprintf(stderr, "SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
        return 1;
    }

    SDL_Event e;
    // Create and initialize your R-tree
    R_TREE r_tree = create_new_r_tree();



    // Render the R-Tree visualization
    bool quit = false;
    int flag = 0;

    while (!quit) {
        flag = 0;
        int choice;
        printf("Choose an option:\n");
        printf("1. Insert object from file\n");
        printf("2. Insert object manually\n");
        printf("3. Find the objects within radius\n");
        printf("4. Get Nearest Neighbor\n");
        printf("5. K Neighbors search\n");
        printf("Enter your choice: ");
        scanf("%d", &choice);

        switch (choice) {
            case 1: {
                char filename[100];
                printf("Enter the file name: ");
                scanf("%s", filename);
                FILE *objects_file = fopen(filename, "r");
                if (objects_file == NULL) {
                    fprintf(stderr, "Error opening objects file!\n");
                    break;
                }
                int x, y;
                char name[100];
                while (fscanf(objects_file, "%d %d %s", &x, &y, name) != EOF) {
                    insert_in_r_tree(r_tree, create_new_object(x, y, name));
                }
                fclose(objects_file);
                printf("R-tree structure:\n");
                pre_order_traversal(r_tree->root, 0);

                break;
            }
            case 2: {
                int x, y;
                char name[100];
                printf("Enter x y name: ");
                scanf("%d %d %s", &x, &y, name);
                insert_in_r_tree(r_tree, create_new_object(x, y, name));
                printf("R-tree structure:\n");
                pre_order_traversal(r_tree->root, 0);
                break;
            }
            case 3: {
                int user_x, user_y;
                double radius;
                printf("Enter user's coordinates (x y): ");
                scanf("%d %d", &user_x, &user_y);
                printf("Enter the radius: ");
                scanf("%lf", &radius);



                // Render the user's coordinates as a point
                SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0x00, 0xFF); // Yellow color
                SDL_RenderDrawPoint(renderer, user_x, user_y);
                SDL_Rect rect = {user_x - 2, user_y - 2, 4, 4}; // Adjust the size as needed
                SDL_SetRenderDrawColor(renderer, 0x00, 0xFF, 0x00, 0xFF); // Green color
                SDL_RenderFillRect(renderer, &rect);
                SDL_RenderPresent(renderer);

                // Search objects within the radius
                OBJ found_objects[MAX_OBJECTS];
                int num_found = 0;
                RECT search_rect = create_new_rect(user_x - radius, user_y - radius, user_x + radius, user_y + radius);
                search_in_r_tree(r_tree->root, search_rect , user_x, user_y, radius, found_objects, &num_found);

                printf("%d - " , num_found);

                printf("Objects found within the radius: \n");
                for (int i = 0; i < num_found; ++i) {
                    OBJ object = found_objects[i];
                    printf(" Object %d: (%d, %d)\n ", i + 1, object->x, object->y);
                }

                render_points_within_radius(user_x , user_y , found_objects, num_found, radius);
                flag = 1;
                break;
                }
            case 4: {
                // Get user coordinates
                int user_x, user_y;
                printf("Enter user's coordinates (x y): ");
                scanf("%d %d", &user_x, &user_y);

                // Render the user's coordinates as a point
                SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0x00, 0xFF); // Yellow color
                SDL_RenderDrawPoint(renderer, user_x, user_y);
                SDL_Rect rect = {user_x - 2, user_y - 2, 4, 4}; // Adjust the size as needed
                SDL_SetRenderDrawColor(renderer, 0x00, 0xFF, 0x00, 0xFF); // Green color
                SDL_RenderFillRect(renderer, &rect);
                SDL_RenderPresent(renderer);

                // Search for nearest neighbor
                OBJ nearest_neighbor = NULL;
                double min_distance = INT_MAX;
                nearest_neighbor = search_nearest_neighbor(r_tree->root, bounding_box(r_tree->root), user_x, user_y, nearest_neighbor, &min_distance);

                // Render nearest neighbor's point on the visualization
                if (nearest_neighbor != NULL) {
                    SDL_SetRenderDrawColor(renderer, 0xFF, 0x00, 0x00, 0xFF); // Red color
                    SDL_RenderDrawPoint(renderer, nearest_neighbor->x, nearest_neighbor->y);
                    SDL_RenderPresent(renderer);
                    printf("Nearest Point - (%d , %d)\n" , nearest_neighbor->x, nearest_neighbor->y);

                    // Draw a line between the user's point and the nearest neighbor
                    SDL_SetRenderDrawColor(renderer, 0x00, 0xFF, 0x00, 0xFF); // Green color for the line
                    SDL_RenderDrawLine(renderer, user_x, user_y, nearest_neighbor->x, nearest_neighbor->y);
                    SDL_RenderPresent(renderer);
                }
                flag = 1;
                break;
            }
            case 5: {
                // Get user coordinates
                int user_x, user_y;
                printf("Enter user's coordinates (x y): ");
                scanf("%d %d", &user_x, &user_y);

                // Render the user's coordinates as a point
                SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0x00, 0xFF); // Yellow color
                SDL_RenderDrawPoint(renderer, user_x, user_y);
                SDL_Rect rect = {user_x - 2, user_y - 2, 4, 4}; // Adjust the size as needed
                SDL_SetRenderDrawColor(renderer, 0x00, 0xFF, 0x00, 0xFF); // Green color
                SDL_RenderFillRect(renderer, &rect);
                SDL_RenderPresent(renderer);

                // Search for k-nearest neighbors
                //PriorityQueue* pq = init_priority_queue(K_NEAREST_NEIGHBORS);
                OBJ nearest_neighbors[K_NEAREST_NEIGHBORS] ;
                find_k_nearest_neighbors(r_tree->root,user_x, user_y, K_NEAREST_NEIGHBORS , nearest_neighbors);

                // Render k-nearest neighbors' points on the visualization
                SDL_SetRenderDrawColor(renderer, 0xFF, 0x00, 0x00, 0xFF); // Red color
                printf("Nearest Neighbors\n");
                for (int i = 0; i < K_NEAREST_NEIGHBORS; ++i) {
                    if (nearest_neighbors[i] != NULL) {
                        SDL_SetRenderDrawColor(renderer, 0xFF, 0x00, 0x00, 0xFF); // Red color
                        SDL_RenderDrawPoint(renderer, nearest_neighbors[i]->x, nearest_neighbors[i]->y);
                        printf(" (%d ,%d)" ,  nearest_neighbors[i]->x, nearest_neighbors[i]->y);
                        SDL_RenderPresent(renderer);

                        // Draw a line between the user's point and the nearest neighbor
                        SDL_SetRenderDrawColor(renderer, 0x00, 0xFF, 0x00, 0xFF); // Green color for the line
                        SDL_RenderDrawLine(renderer, user_x, user_y, nearest_neighbors[i]->x, nearest_neighbors[i]->y);
                        SDL_RenderPresent(renderer);
                    }
                }
                SDL_RenderPresent(renderer);

                // Free memory allocated for the priority queue
                flag = 1;
                break;
                }
            case 6: {
                quit = true;
                break;
            }
            default:
                printf("Invalid choice!\n");
        }
        if(flag == 0) {
        // Render the R-Tree visualization
        SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0xFF);
        SDL_RenderClear(renderer);
        render_r_tree_node(r_tree->root, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
        SDL_RenderPresent(renderer);
        }

        // Poll for events
        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_QUIT) {
                // If the user closes the window, set quit to true
                quit = true;
            }
        }
    }

    // Free resources and quit SDL
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
