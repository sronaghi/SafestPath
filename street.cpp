/*Daniel Kuelbs (dkuelbs)- SL is Grant Bishko;
 * Sasha Ronaghi (sronaghi)- SL is Tommy Yang.
 *This file contains code for the final project, which finds
 *the safest path through a city.
 */

#include <string>
#include "grid.h"
#include "testing/SimpleTest.h"
#include "error.h"

#include "vector.h"
#include "strlib.h"
#include "priorityqueue.h"
#include "street.h"

#include "set.h"
#include "queue.h"
#include "stack.h"

using namespace std;

/**
 * @brief areEqual. This function takes two paths and
 * determines if they are eqaul by comparing their light,
 * density, crime, and sidewalk status.
 * @param path1 is the first path to be compared
 * @param path2 is the second path to be compared
 * @return true if path1 and path2 are equal and
 * false if path1 and path2 are not equal.
 */
bool areEqual(Vector<street> path1, Vector<street> path2){
    if (path1.size() != path2.size()){
        return false;
    }
    for (int i = 0; i < path1.size(); i++){
        if (path1[i].getCrime() != path2[i].getCrime()
                || path1[i].getLight() != path2[i].getLight()
                || path1[i].getDensity() != path2[i].getDensity()
                || (!path1[i].isSidewalk() && path2[i].isSidewalk())
                || (path1[i].isSidewalk() && !path2[i].isSidewalk())){
            return false;
        }
    }
    return true;
}

/**
 * @brief getPathSafetyVector returns the safety rating
 * of the entire path
 * @param path that is a Vector<street> for which is the
 * safety rating is needed
 * @return an int of the safety rating of the entire path
 */
int getPathSafetyVector(Vector<street> path){
    int output = 0;
    for (street s: path){
        output += s.getSafetyRating();
    }
    return output;
}

/**
  Solution 1
  Let N be the number of rows in the grid and let M be the number of columns.
  Since this function will call itself at most twice, it will have a runtime of O((NM)^2).
  */
/**
 * @brief safestPath1Helper is a helper function that uses recursion to
 * find all of the valid paths in the city and inserts them in a
 * priority queue based on their safety rating
 * @param city is a Grid of streets that is analyzed to find the safest path
 * @param row is an int in the current row position of the street
 * at which the recursive function is at.
 * @param col is an int in the current col position of the street
 * at which the recursive function is at.
 * @param path is the current path the recursive function is taking
 * through the city
 * @param solutions is a PriorityQueue<Vector<street>> that is passed
 * by reference is holds all of the valid paths as well as their safety
 * rating as the path's priority value
 */
void safestPath1Helper(Grid<street> city, int row, int col, Vector<street> path, PriorityQueue<Vector<street>>& solutions){
    if (row == city.numRows() - 1 && col == city.numCols() - 1){
        solutions.enqueue(path, -(getPathSafetyVector(path)));
        //negative because safer paths will have a lower priority value
    }
    else {
        Vector<street> rightPath = path;
        Vector<street> downPath = path;
        if (row < city.numRows() - 1 && city[row + 1][col].isSidewalk()){
            downPath.add(city[row + 1][col]);
            safestPath1Helper(city, row + 1, col, downPath, solutions);
        }
        if (col < city.numCols() - 1 && city[row][col + 1].isSidewalk()){
            rightPath.add(city[row][col + 1]);
            safestPath1Helper(city, row, col + 1, rightPath, solutions);
        }
    }
}

/**
 * @brief safestPath1 returns the Vector<street> of the
 * safest path through the city using a recursive helper
 * function and a priority queue. It finds all possible solutions
 * in the recursive portion and then returns the safest
 * option using the priority queue.
 * @param city is a Grid<street> for which the safest
 * path is wanted to be found
 * @return the Vector<street> of the
 * safest path through the city
 */
Vector<street> safestPath1(Grid<street> city){
    PriorityQueue<Vector<street>> solutions;
    Vector<street> path;
    path.add(city[0][0]);
    safestPath1Helper(city, 0, 0, path, solutions);
    return solutions.dequeue();
}



//Solution 2
/**
 * @brief getSaferPath is a helper function
 * that takes two paths and returns the
 * safer path. It checks that both paths
 * are the same size.
 * @param path1 is a Vector<street> of the
 * first path to be considered.
 * @param path2 is a Vector<street> of the
 * first path to be considered.
 * @return a Vector<street> of the path that is
 * safer or longer.
 */
Vector<street> getSaferPath(Vector<street> path1, Vector<street> path2){
    if (path1.size() > path2.size()){
        return path1;
    }
    else if (path2.size() > path1.size()){
        return path2;
    }
    else {
        if (getPathSafetyVector(path1) >= getPathSafetyVector(path2)){
            return path1;
        }
        return path2;
    }

}
/**
 * @brief safestPath2Helper is a helper function
 * that uses recursive backtracking to find the safest path in the city
 * and only returns the safest path
 * @param city is a Grid of streets that is analyzed to find the path
 * @param row is an int in the current row position of the street
 * at which the recursive function is at.
 * @param col is an int in the current col position of the street
 * at which the recursive function is at.
 * @param path is the current path the recursive function is taking
 * through the city
 * @return a Vector<street> of the single safest path in the city
 */
Vector<street> safestPath2Helper(Grid<street> city, int row, int col, Vector<street>& path){
    if (row == city.numRows() - 1 && col == city.numCols() - 1){ // end of path
        return path;
    }
    else if (row == city.numRows() - 1){ //last row
        if (!city[row][col + 1].isSidewalk()){
            return {};
        }
        path.add(city[row][col + 1]);
        return safestPath2Helper(city, row, col + 1, path);
    }
    else if (col == city.numCols() - 1){
        if (!city[row + 1][col].isSidewalk()){
            return {};
        }
        path.add(city[row + 1][col]);
        return safestPath2Helper(city, row + 1, col, path);
    }
    else {
        Vector<street> rightPath = path;
        Vector<street> downPath = path;
        if (city[row + 1][col].isSidewalk()){
            downPath.add(city[row + 1][col]);
        }
        if (city[row][col + 1].isSidewalk()){
            rightPath.add(city[row][col + 1]);
        }
        rightPath = safestPath2Helper(city, row, col + 1, rightPath);
        downPath = safestPath2Helper(city, row + 1, col, downPath);
        return getSaferPath(rightPath, downPath);
    }
    return {};
}

/**
 * @brief safestPath2 returns the Vector<street> of the
 * safest path through the city using a recursive helper
 * function.
 * @param city is a Grid<street> for which the safest
 * path is wanted to be found
 * @return the Vector<street> of the
 * safest path through the city
 */

Vector<street> safestPath2(Grid<street> city){
    Vector<street> path = {};
    path.add(city[0][0]);
    return safestPath2Helper(city, 0, 0, path);
}


//Solution 3

/**
 * @brief generateValidMoves is function that takes in
 * the city as both a grid of GridLocation and streets
 * and the current Gridlocation the generates the valid
 * moves from the current location. It considers
 * whether or not the next moves are sidewalks and
 * are in bounds
 * @param city a Grid<GridLocation> passed by reference
 * that is the city for which the next moves can be found in
 * @param cur is a GridLocation of the current location
 * of the function
 * @param cityStreet is Grid<street> passed by reference
 * that is the city for which the next moves can be found in
 * @return is a Set<GridLocation> of the next valid moves from
 * the current location, cur.
 */
Set<GridLocation> generateValidMoves(Grid<GridLocation>& city, GridLocation cur, Grid<street>& cityStreet) {
    int cur_x = cur.row;
    int cur_y = cur.col;
    Vector<GridLocation> possible_locations = {GridLocation(cur_x, cur_y + 1),
                                               GridLocation(cur_x +  1, cur_y)};
    Set<GridLocation> neighbors;
    for (GridLocation location : possible_locations) {
        if (city.inBounds(location) &&
                cityStreet[location.row][location.col].isSidewalk()) {
            neighbors.add(location);
        }
    }
    return neighbors;
}

/**
 * @brief getGridLocPathSafety takes in a path that is a
 * Stack<GridLocation> and returns its safety rating by finding
 * the GridLocation's equivalent street and respective
 * safety rating.
 * @param path is a Stack<GridLocation> for which the safety
 * rating is to be found.
 * @param city is a Grid<street> for which the path's safety rating
 * needs to be found through
 * @return an int that is the path's safety rating.
 */
int getGridLocPathSafety(Stack<GridLocation> path, Grid<street>& city){
    int output = 0;
    while (!path.isEmpty()){
        GridLocation loc = path.pop();
        output += city[loc.row][loc.col].getSafetyRating();

    }
    return output;
}

/**
 * @brief safestPath3Helper is a helper function
 * that iteratively finds the safest path through the city
 * using Gridlocations, Stacks, PriorityQueues, Queues, and Sets.
 * The function takes in a grid of the same city as both a grid of
 * GridLocations and streets. The GridLocation corresponds to the street
 * at the position in the grid. The function finds all possible paths
 * and returns the safest path using a priority queue, where the priority
 * is determined based on the path's safety rating.
 * @param city is a Grid<GridLocation> that is analyzed to find the safest path
 * @param cityStreet is a Grid<street> that is analyzed to find the safest path
 * @return a Stack<GridLocation> of the safest path in the city
 */
Stack<GridLocation> safestPath3Helper(Grid<GridLocation>& city, Grid<street>& cityStreet) {
    Stack<GridLocation> path;
    PriorityQueue<Stack<GridLocation>> solutions;
    Queue<Stack<GridLocation>> paths;
    GridLocation entry = GridLocation(0,0);
    GridLocation exit = {city.numRows()-1,  city.numCols()-1};
    path.push(entry);
    paths.enqueue(path);
    Set<GridLocation> checked_moves;
    while (paths.size() > 0){
        Stack<GridLocation> curr_path = paths.dequeue();
        checked_moves.add(curr_path.peek());
        if (curr_path.peek() == exit) {
            solutions.enqueue(curr_path, -(getGridLocPathSafety(curr_path, cityStreet)));
            //negative because safer paths will have a lower priority value
        }
        else{
            Set<GridLocation> valid_moves = generateValidMoves(city, curr_path.peek(), cityStreet);
            for (GridLocation move : valid_moves) {
                if (!checked_moves.contains(move)) {
                    Stack<GridLocation> new_path = curr_path;
                    new_path.push(move);
                    paths.enqueue(new_path);
                }
            }
        }

    }

    return solutions.dequeue();

}
/**
 * @brief safestPath3 is an iterative function that
 * uses a helper function to find the safest path through
 * the city. The function creates a new Grid<GridLocation>, in
 * which each GridLocation corresponds to a street. It uses a
 * helper function to find the safest path as a Stack<GridLocation>
 * and converts it to a Vector<street>.
 * @param cityStreet is a Grid<street> that is
 * analyzed to find the safest path
 * @return the Vector<street> of the
 * safest path through the city
 *
 *  Let N be the number of rows in the grid and let M be the number of columns.
 *  Then the runtime of this function is O((NM)(N + M - 1)^2), since it goes through each row and column during the iteration.
 *  Then, the function does vector insertion, which is 0(k^2) where k is the size of the vector
 *  because the vector moves all of the elements for each insert. The vector size is (N + M - 1).
 */
Vector<street> safestPath3(Grid<street>& cityStreet){
    Grid<GridLocation> city(cityStreet.numRows(), cityStreet.numCols());
    Stack<GridLocation> stackOutput = safestPath3Helper(city, cityStreet);
    Vector<street> output;
    while (!stackOutput.isEmpty()){
        GridLocation loc = stackOutput.pop();
        output.insert(0, cityStreet[loc.row][loc.col]);
    } //reverse vector function
    return output;
}

//TESTING


STUDENT_TEST("Very simple example"){
    street sdwlk =  street(2, 3,  4, false);
    street street1 =  street(10, 1, 1, true);

    Grid<street> city = {{street1, sdwlk},
                         {street1, street1}};
    Vector<street> expectedPath = {street1, street1, street1};

    Vector<street> actual1 = safestPath1(city);
    Vector<street> actual2 = safestPath2(city);
    Vector<street> actual3 = safestPath2(city);


    EXPECT(areEqual(expectedPath, actual1));
    EXPECT(areEqual(expectedPath, actual2));
    EXPECT(areEqual(expectedPath, actual3));

    //n is total number of elements in the grid
    TIME_OPERATION(4, safestPath1(city));
    TIME_OPERATION(4, safestPath2(city));
    TIME_OPERATION(4, safestPath3(city));
}

STUDENT_TEST("Simple example"){
    street sdwlk =  street(2, 3,  4, false);
    street street1 =  street(10, 1, 1, true);
    street street2 =  street(0, 0, 0, true);

    Grid<street> city = {{street2, street1, street2},
                         {street2, sdwlk, street2},
                         {street2, street2, street2}};
    Vector<street> expectedPath = {street2, street1, street2, street2, street2};

    Vector<street> actual1 = safestPath1(city);
    Vector<street> actual2 = safestPath2(city);
    Vector<street> actual3 = safestPath2(city);


    EXPECT(areEqual(expectedPath, actual1));
    EXPECT(areEqual(expectedPath, actual2));
    EXPECT(areEqual(expectedPath, actual3));

    TIME_OPERATION(9, safestPath1(city));
    TIME_OPERATION(9, safestPath2(city));
    TIME_OPERATION(9, safestPath3(city));
}

STUDENT_TEST("Complicated example"){
    street sdwlk =  street(2, 3,  4, false);
    street street1 =  street(10, 1, 1, true);
    street street2 =  street(0, 0, 0, true);

    Grid<street> city = {{street2, street1, street2, street1, street2},
                         {street2, sdwlk, street2, street1, street2},
                         {street2, street2, street2, street1, street1},
                         {street2, street2, sdwlk, street1, street1},
                         {sdwlk, street2, sdwlk, street2, street2}};
    Vector<street> expectedPath = {street2, street1, street2, street1, street1, street1, street1, street1, street2};
    Vector<street> actual1 = safestPath1(city);
    Vector<street> actual2 = safestPath2(city);
    Vector<street> actual3 = safestPath2(city);

    EXPECT(areEqual(expectedPath, actual1));
    EXPECT(areEqual(expectedPath, actual2));
    EXPECT(areEqual(expectedPath, actual3));

    TIME_OPERATION(25, safestPath1(city));
    TIME_OPERATION(25, safestPath2(city));
    TIME_OPERATION(25, safestPath3(city));
}

STUDENT_TEST("More complicated example"){
    street sdwlk =  street(2, 3,  4, false);
    street street1 =  street(10, 1, 1, true);
    street street2 =  street(0, 0, 0, true);
    street street3 = street(50, 1, 60, true);
    street street4 = street (0, 15, 3, true);


    Grid<street> city = {{street2, street3, street2, street1, street2, sdwlk},
                         {street2, sdwlk, street3, street4, street2, street1},
                         {street2, street2, street2, street1, street3, street4},
                         {street2, street2, sdwlk, street1, street1, street3},
                         {sdwlk, street2, sdwlk, street2, street2, street3}};
    Vector<street> expectedPath = {street2, street3, street2, street3, street2, street1, street3, street1, street3, street3};
    Vector<street> actual1 = safestPath1(city);
    Vector<street> actual2 = safestPath2(city);
    Vector<street> actual3 = safestPath2(city);

    EXPECT(areEqual(expectedPath, actual1));
    EXPECT(areEqual(expectedPath, actual2));
    EXPECT(areEqual(expectedPath, actual3));

    TIME_OPERATION(30, safestPath1(city));
    TIME_OPERATION(30, safestPath2(city));
    TIME_OPERATION(30, safestPath3(city));
}


STUDENT_TEST("Two paths of same safety rating"){
    street sdwlk =  street(2, 3,  4, false);
    street street1 =  street(3, 2, 1, true);
    street street2 =  street(5, 1, 4, true);

    Grid<street> city = {{street2, street2, street1},
                         {street1, sdwlk, street2},
                         {street1, street1, street2}};
    Vector<street> expectedPath1 = {street2, street2, street1, street2, street2};
    Vector<street> expectedPath2 = {street2, street1, street1, street1, street2};

    Vector<street> actual1 = safestPath1(city);
    Vector<street> actual2 = safestPath2(city);
    Vector<street> actual3 = safestPath2(city);


    EXPECT(areEqual(expectedPath1, actual1) || areEqual(expectedPath2, actual1));
    EXPECT(areEqual(expectedPath1, actual2) || areEqual(expectedPath2, actual2));
    EXPECT(areEqual(expectedPath1, actual3) || areEqual(expectedPath2, actual3));
}

