#include "bits.h"
#include "error.h"
#include <string>
#include <vector>
using namespace std;

/**
 * Routines for managing Bit class and for reading/write Bits within EncodedData
 * objects to a stream.  The public interface is provided in bits.h header file.
 * You do not need to revew the implementation details in this file but are welcome
 * to take a peek if you're curious.
 */

Bit::Bit(int value) {
    /* Check for use of chararacter values. */
    if (value == '0' || value == '1') {
        error("You have attempted to create a bit equal to the character '0' or '1'. "
              "The characters '0' and '1' are not the same as the numbers 0 and 1. "
              "Edit your code to instead use the numeric values 0 and 1 instead.");
    }
    if (value != 0 && value != 1) {
        error("Illegal value for a bit: " + to_string(value));
    }

    _value = (value == 1);
}

bool operator== (Bit lhs, Bit rhs) {
    return lhs._value == rhs._value;
}
bool operator!= (Bit lhs, Bit rhs) {
    return !(lhs == rhs);
}
ostream& operator<< (ostream& out, Bit bit) {
    return out << (bit._value? '1' : '0');
}


namespace {
    /**
     * Validates that the given EncodedData obeys all the invariants we expect it to.
     */
    void checkIntegrityOf(const EncodedData& data) {
        /* Number of distinct characters must be at least two. */
        if (data.treeLeaves.size() < 2) {
            error("File must contain at least two distinct characters.");
        }

        /* Number of bits in tree shape should be exactly 2c - 1, where c is the number of
         * distinct characters.
         */
        if (data.treeShape.size() != data.treeLeaves.size() * 2 - 1) {
            error("Wrong number of tree bits for the given leaves.");
        }
    }

    /* Utility types for reading/writing individual bits. Inspired by a
     * similar implementation by Julie Zelenski.
     */
    class BitWriter {
    public:
        explicit BitWriter(ostream& o) : _out(o) {}
        ~BitWriter() {
            if (_bitIndex != 0) flush();
        }

        void put(Bit b) {
            if (b != 0) {
                _bitBuffer |= (1U << _bitIndex);
            }

            _bitIndex++;
            if (_bitIndex == 8) {
                flush();
            }
        }

    private:
        void flush() {
            _out.put(_bitBuffer);
            _bitBuffer = 0;
            _bitIndex = 0;
        }

        ostream& _out;
        uint8_t _bitBuffer = 0;
        uint8_t _bitIndex  = 0;
    };

    class BitReader {
    public:
        explicit BitReader(istream& i) : _in(i) {}

        Bit get() {
            if (_bitIndex == 8) readMore();

            Bit result = !!(_bitBuffer & (1U << _bitIndex));
            _bitIndex++;
            return result;
        }

    private:
        istream& _in;
        uint8_t _bitBuffer = 0;
        uint8_t _bitIndex  = 8;

        void readMore() {
            char read;
            if (!_in.get(read)) {
                error("Unexpected end of file when reading bits.");
            }

            _bitBuffer = read;
            _bitIndex = 0;
        }
    };

    /* "CS106B A7" */
    const uint32_t kFileHeader = 0xC5106BA7;
}

/**
 * We store EncodedData on disk as follows:
 *
 *
 * 1 byte:  number of distinct characters, minus one.
 * c bytes: the leaves of the tree, in order.
 * 1 byte:  number of valid bits in the last byte.
 * n bits:  tree bits, followed by message bits.
 *
 * We don't need to store how many bits are in the tree, since it's always given
 * by 2*c - 1, as this is the number of nodes in a full binary tree with c leaves.
 */
void writeData(EncodedData& data, ostream& out) {
    /* Validate invariants. */
    checkIntegrityOf(data);

    /* Write magic header. */
    out.write(reinterpret_cast<const char *>(&kFileHeader), sizeof kFileHeader);

    /* Number of characters. */
    const uint8_t charByte = data.treeLeaves.size() - 1;
    out.put(charByte);

    /* Tree leaves. */
    while (!data.treeLeaves.isEmpty()) out.put(data.treeLeaves.dequeue());

    /* Number of bits in the last byte to read. */
    uint8_t modulus = (data.treeShape.size() + data.messageBits.size()) % 8;
    if (modulus == 0) modulus = 8;
    out.put(modulus);

    /* Bits themselves. */
    BitWriter writer(out);
    while (!data.treeShape.isEmpty()) writer.put(data.treeShape.dequeue());
    while (!data.messageBits.isEmpty()) writer.put(data.messageBits.dequeue());
}

/**
 * Reads EncodedData from stream.
 */
EncodedData readData(istream& in) {
    /* Read back the magic header and make sure it matches. */
    uint32_t header;
    if (!in.read(reinterpret_cast<char *>(&header), sizeof header) ||
        header != kFileHeader) {
        error("Chosen file is not a Huffman-compressed file.");
    }

    EncodedData data;

    /* Read the character count. */
    char skewCharCount;
    if (!in.get(skewCharCount)) {
        error("Error reading character count.");
    }

    /* We offset this by one - add the one back. */
    int charCount = uint8_t(skewCharCount);
    charCount++;

    if (charCount < 2) {
        error("Character count is too low for this to be a valid file.");
    }

    /* Read in the leaves. */
    vector<char> leaves(charCount);
    if (!in.read(leaves.data(), leaves.size())) {
        error("Could not read in all tree leaves.");
    }
    for (char leaf: leaves) {
        data.treeLeaves.enqueue(leaf);
    }

    /* Read in the modulus. */
    char signedModulus;
    if (!in.get(signedModulus)) {
        error("Error reading modulus.");
    }
    uint8_t modulus = signedModulus;

    /* See how many bits we need to read. To do this, jump to the end of the file
     * and back to where we are to count the bytes, then transform that to a number
     * of bits.
     *
     * Thanks to Julie Zelenski for coming up with this technique!
     */
    auto currPos = in.tellg();
    if (!in.seekg(0, istream::end)) {
        error("Error seeking to end of file.");
    }
    auto endPos  = in.tellg();
    if (!in.seekg(currPos, istream::beg)) {
        error("Error seeking back to middle of file.");
    }

    /* Number of bits to read = (#bytes - 1) * 8 + modulus. */
    uint64_t bitsToRead = (endPos - currPos - 1) * 8 + modulus;

    /* Read in the tree shape bits. */
    BitReader reader(in);
    for (int i = 0; i < 2 * charCount - 1; i++) {
        data.treeShape.enqueue(reader.get());
        bitsToRead--;
    }

    /* Read in the message bits. */
    while (bitsToRead > 0) {
        data.messageBits.enqueue(reader.get());
        bitsToRead--;
    }

    return data;
}

/* For debugging purposes. */
ostream& operator<< (ostream& out, const EncodedData& data) {
    ostringstream builder;
    builder << "{treeShape:" << data.treeShape
            << ",treeLeaves:" << data.treeLeaves
            << ",messageBits:" << data.messageBits
            << "}";
    return out << builder.str();
}



//#include <string>
//#include "grid.h"
//#include "stack.h"
//#include "testing/MemoryDiagnostics.h"
//#include "testing/SimpleTest.h"
//#include "error.h"
//#include "map.h"
//#include "vector.h"
//#include "strlib.h"
//#include "priorityqueue.h"
//#include "set.h"
//#include "street.h"

//using namespace std;

//bool areEqual(Vector<Street> path1, Vector<Street> path2){
//    if (path1.size() != path2.size()){
//        return false;
//    }
//    for (int i = 0; i < path1.size(); i++){
//        if (path1[i].getCrime() != path2[i].getCrime()
//                || path1[i].getLight() != path2[i].getLight()
//                || path1[i].getDensity() != path2[i].getDensity()
//                || (!path1[i].isSidewalk() && path2[i].isSidewalk())
//                || (path1[i].isSidewalk() && !path2[i].isSidewalk())){
//            return false;
//        }
//    }
//    return true;
//}

//Solution 1
//int getPathSafetyVector(Vector<Street> path){
//    int output = 0;
//    for (Street s: path){
//        output += s.getSafetyRating();
//    }
//    return output;
//}


//void findAllPathsHelper(Grid<Street> city, int row, int col, Vector<Street> path, PriorityQueue<Vector<Street>>& solutions){
//    if (row == city.numRows() - 1 && col == city.numCols() - 1){
//        solutions.enqueue(path, -(getPathSafetyVector(path)));
//    }
//    else {
//        Vector<Street> leftPath = path;
//        Vector<Street> downPath = path;
//        if (row < city.numRows() - 1 && city[row + 1][col].isSidewalk()){
//            downPath.add(city[row + 1][col]);
//            findAllPathsHelper(city, row + 1, col, downPath, solutions);
//        }
//        if (col < city.numCols() - 1 && city[row][col + 1].isSidewalk()){
//            leftPath.add(city[row][col + 1]);
//            findAllPathsHelper(city, row, col + 1, leftPath, solutions);
//        }
//    }
//}


////void findAllPaths(Grid<Street> city, PriorityQueue<Vector<Street>>& solutions){
////    Vector<Street> path = {};
////    findAllPathsHelper(city, 0, 0, path, solutions);
////}

//Vector<Street> safestPath1(Grid<Street> city){
//    PriorityQueue<Vector<Street>> solutions;
//    Vector<Street> path = {};
//    path.add(city[0][0]);
//    findAllPathsHelper(city, 0, 0, path, solutions);
//    PriorityQueue<Vector<Street>> solutions1 = solutions;
//    cout << solutions1.size() << endl;
//    cout << -getPathSafetyVector(solutions1.dequeue()) << " " << -getPathSafetyVector(solutions1.dequeue()) << endl;
//    return solutions.dequeue();
//}


//Solution 1
//int getPathSafetyVector(Vector<Street> path){
//    int output = 0;
//    for (Street s: path){
//        output += s.safetyRating();
//    }
//    return output;
//}


//void findAllPathsHelper(Grid<int> city, int row, int col, Vector<Street> path, PriorityQueue<Vector<Street>>& solutions){
//    if (row == city.numRows() - 1 && col == city.numCols() - 1){
//        solutions.enqueue(path, getPathSafetyVector(path));
//    }
//    else {
//        Vector<Street> leftPath = path;
//        Vector<Street> downPath = path;
//        if (row < city.numRows() - 1){
//            downPath.add(city[row + 1][col]);
//            findAllPathsHelper(city, row + 1, col, downPath, solutions);
//        }
//        if (col < city.numCols() - 1){
//            leftPath.add(city[row][col + 1]);
//            findAllPathsHelper(city, row, col + 1, leftPath, solutions);
//        }
//    }
//}


//void findAllPaths(Grid<int> city, PriorityQueue<Vector<Street>>& solutions){
//    Vector<Street> path = {};
//    return findAllPathsHelper(city, 0, 0, path, solutions);
//}

//Vector<Street> safestPath1(Grid<int> city){
//    PriorityQueue<Vector<Street>> solutions;
//    findAllPaths(city, solutions);
//    return solutions.dequeue();
//}


//Solution 2


//Vector<Street> safestPath2(Grid<Street> city, int row, int col, Vector<Street> path){
//    if (row == city.numRows() - 1 && col == city.numCols() - 1){
//        return path;
//    }
//    else if (row == city.numRows() - 1){
//        if (!city[row][col + 1].isSidewalk()){
//            return {};
//        }
//        path.add(city[row][col + 1]);
//        safestPath2(city, row, col + 1, path);
//    }
//    else if (col == city.numCols() - 1){
//        if (!city[row + 1][col].isSidewalk()){
//            return {};
//        }
//        path.add(city[row + 1][col]);
//        safestPath2(city, row + 1, col, path);
//    }
//    else {
//        Vector<Street> leftPath = path;
//        Vector<Street> downPath = path;
//        if (row < city.numRows() - 1 && city[row + 1][col].isSidewalk()){
//            downPath.add(city[row + 1][col]);
//        }
//        if (col < city.numCols() - 1 && city[row][col + 1].isSidewalk()){
//            leftPath.add(city[row][col + 1]);

//        }
//        if (getPathSafetyVector(leftPath) > getPathSafetyVector(downPath)){
//            safestPath2(city, row, col + 1, leftPath);
//        }
//        else {
//            safestPath2(city, row + 1, col, downPath);
//        }
//    }
//    return {};
//}




// Solution 3

//Vector<Street> safestPath3(Grid<Street> city, int row, int col, Vector<Street> path){
//    if (row == city.numRows() - 1 && col == city.numCols() - 1){
//        return path;
//    }
//    else if (row == city.numRows() - 1){
//        if (!city[row][col + 1].isSidewalk()){
//            return {};
//        }
//        path.add(city[row][col + 1]);
//        safestPath2(city, row, col + 1, path);
//    }
//    else if (col == city.numCols() - 1){
//        if (!city[row + 1][col].isSidewalk()){
//            return {};
//        }
//        path.add(city[row + 1][col]);
//        safestPath2(city, row + 1, col, path);
//    }
//    else {
//        Vector<Street> leftPath = path;
//        Vector<Street> downPath = path;
//        if (row < city.numRows() - 1 && city[row + 1][col].isSidewalk()){
//            downPath.add(city[row + 1][col]);
//        }
//        if (col < city.numCols() - 1 && city[row][col + 1].isSidewalk()){
//            leftPath.add(city[row][col + 1]);

//        }
//        if (getPathSafetyVector(leftPath) > getPathSafetyVector(downPath)){
//            safestPath2(city, row, col + 1, leftPath);
//        }
//        else {
//            safestPath2(city, row + 1, col, downPath);
//        }
//    }
//}
//int getPathSafetyStack(Stack<Street> path){
//    int output = 0;
//    while (!path.isEmpty()){
//        output += safetyRating(path.pop());
//    }
//    return output;
//}

//Set<Street> generateValidMoves(Grid<Street>& city, int row, int col) {
//    Vector<Street> possible_locations = {city[row + 1][col], city[row][col + 1]};
//    Set<Street> neighbors;
//    for (Street location : possible_locations) {
//        if (city.inBounds(location) && location.isSidewalk()) { //not sure why
//            neighbors.add(location);
//        }
//    }
//    return neighbors;
//}

//Stack<Street> solveMaze(Grid<Street>& city) {
//    Stack<Street> path;
//    PriorityQueue<Stack<Street>> paths;
//    Street entry = city.get(0,0);
//    Street exit = city.get(city.numRows()-1,  city.numCols()-1);
//    path.push(entry);
//    paths.enqueue(path, getPathSafetyStack(path));
//    Set<Street> checked_moves;
//    while (paths.size() > 0){
//        Stack<Street> curr_path = paths.dequeue();
//        checked_moves.add(curr_path.peek());

//        if (curr_path.peek() == exit) {
//            return curr_path;
//        }
//        Set<Street> valid_moves = generateValidMoves(city, curr_path.peek().getRow(), curr_path.peek().getCol());
//        for (Street move : valid_moves) {
//            if (!checked_moves.contains(move)) {
//                Stack<Street> new_path = curr_path;
//                new_path.push(move);
//                paths.enqueue(new_path, getPathSafetyStack(new_path));
//            }
//        }
//    }
//    error ("Shouldn't end up here"); //the solution should be found in the while loop
//}

//TESTING


//    Grid<Street> createCity(Vector<Street> Streets){
//        int rows = Streets.size() / 2;
//        int cols = Streets.size() / 2;
//        if (Streets.size() % 2 != 0){
//            rows+=1;
//        }
//        Grid<Street> city = Grid<Street>(rows,cols);
//        return Grid<Street>(rows,cols);

//    }



//    Vector<Street> comparePaths(Vector<Street> path1, Vector<Street> path2){
//        if (path1.size() > path2.size()){
//            return path1;
//        }
//        else if (path2.size() > path1.size()){
//            return path2;
//        }
//        else {
//            int safety1 = 0;
//            for (Street s: path1){
//                safety1 += safetyRating(s);
//            }
//            int safety2 = 0;
//            for (Street s: path2){
//                safety2 += safetyRating(s);
//            }
//            if (safety1 >= safety2){
//                return path1;
//            }
//            return path2;
//        }

//    }

//    STUDENT_TEST("Simple example"){
//        Street sdwlk = new Street(false);
//        Street Street1 = new Street(10, 1, 1, true);
//        Street Street2 = new Street(0, 0, 0, true);
//        Grid<int> city = {{Street2.safetyRating(), Street1.safetyRating(), Street2.safetyRating()},
//                             {Street2.safetyRating(), sdwlk.safetyRating(), Street2.safetyRating()},
//                             {Street2.safetyRating(), Street2.safetyRating(), Street2.safetyRating()}};
//        Vector<Street> expectedPath = {Street2, Street1, Street2, Street2, Street2};

//        //EXPECT_EQUAL(safestPath1(city), expectedPath);
//        Vector<Street> actual = safestPath1(city);


//        EXPECT(areEqual(expectedPath, safestPath1(city)));
//    }

//STUDENT_TEST("Simple example"){
//    Street sdwlk =  Street(2, 3,  4, false);
//    Street Street1 =  Street(10, 1, 1, true);
//    Street Street2 =  Street(0, 0, 0, true);

//    Grid<Street> city = {{Street2, Street1, Street2},
//                         {Street2, sdwlk, Street2},
//                         {Street2, Street2, Street2}};
//    Vector<Street> expectedPath = {Street2, Street1, Street2, Street2, Street2};

//    //EXPECT_EQUAL(safestPath1(city), expectedPath);
//    Vector<Street> actual = safestPath1(city);
//    for (int i = 0; i < actual.size(); i++) {
//        cout << "actual output//"
//                " light: " << actual[i].getLight() <<
//                " crime: " << actual[i].getCrime() <<
//                " density: " << actual[i].getDensity() << " " <<
//                " safety rating: " << actual[i].getSafetyRating() <<
//                endl;
//        cout << "expected output//"
//                " light: " << expectedPath[i].getLight() <<
//                " crime: " << expectedPath[i].getCrime() <<
//                " density: " << expectedPath[i].getDensity() << " " <<
//                " safety rating: " << expectedPath[i].getSafetyRating() <<
//                endl;
//    }

//    EXPECT(areEqual(expectedPath, safestPath1(city)));
//}
//    void findAllPathsHelper2(Grid<Street> city, int row, int col, Vector<Street> path, PriorityQueue<Vector<Street>>& solutions){

//        if (row == city.numRows() - 1 && col == city.numCols() - 1){
//            solutions.enqueue(path, getPathSafety(path));
//        }
//        else {
//            Vector<Street> leftPath = path;
//            Vector<Street> downPath = path;
//            if (!city[row + 1][col].isSidewalk() && !city[row][col + 1].isSidewalk()){
//                return;
//            }
//            if (row < city.numRows() - 1 && city[row + 1][col].isSidewalk()){
//                downPath.add(city[row + 1][col]);
//                for (int row = city.numRows(); row >= 0; r--){
//                    for (int col = city.numCols(); col >= 0; c--){
//                        findAllPathsHelper(city, row + 1, col, downPath, solutions);
//                    }
//                    if (col < city.numCols() - 1 && city[row][col + 1].isSidewalk()){
//                        leftPath.add(city[row][col + 1]);
//                        findAllPathsHelper(city, row, col + 1, leftPath, solutions);
//                    }
//                }

//                Stack<soFarAndRest> stack;
//                stack.push({"", s});
//                while (!stack.isEmpty()) {

//                    soFarAndRest partial = stack.pop();
//                    string soFar = partial.soFar;
//                    string rest = partial.rest;
//                    if (row == city.numRows() - 1 && col == city.numCols() - 1) {
//                        solutions.enqueue(path, getPathSafety(path));
//                    }
//                    for (int i = 0; i < rest.length(); i++) {
//                        string remaining = rest.substr(0, i) + rest.substr(i + 1);
//                        stack.push({soFar + rest[i], remaining});
//                    }
//                }

//                while (row == city.numRows() - 1 && col == city.numCols() - 1){

//                }

//                for (int row = city.numRows(); row >= 0; r--){
//                    for (int col = city.numCols(); col >= 0; c--){

//                    }
//                }

//Vector<street> safestPath2Iterative(Grid<street> city, int row, int col, Vector<street>& path){
//    while (!row == city.numRows() - 1 || !col == city.numCols() - 1){ // end of path
//    }

//    return {};
//}

//void safestPath2Helper(Grid<street> city, int row, int col, Vector<street>& path){
//    if (row != city.numRows() - 1 || col != city.numCols() - 1){ // end of path
//        if (row == city.numRows() - 1){ //hit the side wall
//            if (city[row][col + 1].isSidewalk()){
//                path.add(city[row][col + 1]);
//                safestPath2Helper(city, row, col + 1, path);
//            }
//        } else if (col == city.numCols() - 1){
//            if (city[row + 1][col].isSidewalk()){
//                path.add(city[row + 1][col]);
//                safestPath2Helper(city, row + 1, col, path);
//            }
//        }
//        else {
//            Vector<street> leftPath = path;
//            Vector<street> downPath = path;
//            if (row < city.numRows() - 1 && city[row + 1][col].isSidewalk()){
//                downPath.add(city[row + 1][col]);
//            }
//            if (col < city.numCols() - 1 && city[row][col + 1].isSidewalk()){
//                leftPath.add(city[row][col + 1]);

//            }
//            if (getPathSafetyVector(leftPath) >= getPathSafetyVector(downPath)){
//                safestPath2Helper(city, row, col + 1, leftPath);
//            }
//            else {
//                safestPath2Helper(city, row + 1, col, downPath);
//            }
//        }
//    }
//}
//    else if (row == city.numRows() - 1){ //hit the side wall
//        if (!city[row][col + 1].isSidewalk()){
//            return {};
//        }
//        path.add(city[row][col + 1]);
//        safestPath2Helper(city, row, col + 1, path);
//    }
//    else if (col == city.numCols() - 1){
//        if (!city[row + 1][col].isSidewalk()){
//            return {};
//        }
//        path.add(city[row + 1][col]);
//        safestPath2Helper(city, row + 1, col, path);
//    }
//    else {
//        Vector<street> leftPath = path;
//        Vector<street> downPath = path;
//        if (row < city.numRows() - 1 && city[row + 1][col].isSidewalk()){
//            downPath.add(city[row + 1][col]);
//        }
//        if (col < city.numCols() - 1 && city[row][col + 1].isSidewalk()){
//            leftPath.add(city[row][col + 1]);

//        }
//        if (getPathSafetyVector(leftPath) >= getPathSafetyVector(downPath)){
//            safestPath2Helper(city, row, col + 1, leftPath);
//        }
//        else {
//            safestPath2Helper(city, row + 1, col, downPath);
//        }
//    }
//    return path;
//}

//Vector<street> safestPath2(Grid<street> city){
//    Vector<street> path;
//    path.add(city[0][0]);
//    safestPath2Helper(city, 0, 0, path);
//    cout << getPathSafetyVector(path) << " " << path.size() << " " << "wooo" << endl;
//    return path;
//}



// Solution 3

//Vector<street> safestPath3(Grid<street> city, int row, int col, Vector<street> path){
//    if (row == city.numRows() - 1 && col == city.numCols() - 1){
//        return path;
//    }
//    else if (row == city.numRows() - 1){
//        if (!city[row][col + 1].isSidewalk()){
//            return {};
//        }
//        path.add(city[row][col + 1]);
//        safestPath2(city, row, col + 1, path);
//    }
//    else if (col == city.numCols() - 1){
//        if (!city[row + 1][col].isSidewalk()){
//            return {};
//        }
//        path.add(city[row + 1][col]);
//        safestPath2(city, row + 1, col, path);
//    }
//    else {
//        Vector<street> leftPath = path;
//        Vector<street> downPath = path;
//        if (row < city.numRows() - 1 && city[row + 1][col].isSidewalk()){
//            downPath.add(city[row + 1][col]);
//        }
//        if (col < city.numCols() - 1 && city[row][col + 1].isSidewalk()){
//            leftPath.add(city[row][col + 1]);

//        }
//        if (getPathSafetyVector(leftPath) > getPathSafetyVector(downPath)){
//            safestPath2(city, row, col + 1, leftPath);
//        }
//        else {
//            safestPath2(city, row + 1, col, downPath);
//        }
//    }
//}
//int getPathSafetyStack(Stack<street> path){
//    int output = 0;
//    while (!path.isEmpty()){
//        output += safetyRating(path.pop());
//    }
//    return output;
//}

//Set<street> generateValidMoves(Grid<GridLocation>& city, int row, int col) {
//    Vector<GridLocation> possible_locations = {city[row + 1][col], city[row][col + 1]};
//    Set<street> neighbors;
//    for (GridLocation location : possible_locations) {
//        if (city.inBounds(location) && location.isSidewalk()) { //not sure why
//            neighbors.add(location);
//        }
//    }
//    return neighbors;
//}


//Stack<street> solveMaze(Grid<street>& city) {
//    Stack<street> path;
//    PriorityQueue<Stack<street>> paths;
//    street entry = city.get(0,0);
//    street exit = city.get(city.numRows()-1,  city.numCols()-1);
//    path.push(entry);
//    paths.enqueue(path, getPathSafetyStack(path));
//    Set<street> checked_moves;
//    while (paths.size() > 0){
//        Stack<street> curr_path = paths.dequeue();
//        checked_moves.add(curr_path.peek());

//        if (curr_path.peek() == exit) {
//            return curr_path;
//        }
//        Set<street> valid_moves = generateValidMoves(city, curr_path.peek().getRow(), curr_path.peek().getCol());
//        for (street move : valid_moves) {
//            if (!checked_moves.contains(move)) {
//                Stack<street> new_path = curr_path;
//                new_path.push(move);
//                paths.enqueue(new_path, getPathSafetyStack(new_path));
//            }
//        }
//    }
//    error ("Shouldn't end up here"); //the solution should be found in the while loop
//}

//Stack<GridLocation> safestPath3Helper(Grid<GridLocation>& city, Grid<street>& cityStreet) {
//    Stack<GridLocation> path;
//    PriorityQueue<Stack<GridLocation>> paths;
//    GridLocation entry = GridLocation(0,0);
//    GridLocation exit = {city.numRows()-1,  city.numCols()-1};
//    path.push(entry);
//    paths.enqueue(path, getGridLocPathSafety(path, cityStreet));
//    Set<GridLocation> checked_moves;
//    while (paths.size() > 0){
//        Stack<GridLocation> curr_path = paths.dequeue();
//        checked_moves.add(curr_path.peek());
//        if (curr_path.peek() == exit) {
//            return curr_path;
//        }
//        Set<GridLocation> valid_moves = generateValidMoves(city, curr_path.peek(), cityStreet);
//        for (GridLocation move : valid_moves) {
//            if (!checked_moves.contains(move)) {
//                Stack<GridLocation> new_path = curr_path;
//                new_path.push(move);
//                paths.enqueue(new_path, getGridLocPathSafety(new_path, cityStreet));
//            }
//        }
//    }
//    error ("Shouldn't end up here"); //the solution should be found in the while loop
//}

//Vector<street> safestPath3(Grid<street>& cityStreet){
//    Grid<GridLocation> city(cityStreet.numRows(), cityStreet.numCols());
//    Stack<GridLocation> stackOutput = safestPath3Helper(city, cityStreet);
//    Vector<street> output;
//    while (!stackOutput.isEmpty()){
//        GridLocation loc = stackOutput.pop();
//        output.insert(0, cityStreet[loc.row][loc.col]);
//    }
//    return output;
//}


/*
 * x
    for (int i = 0; i < expectedPath.size(); i++) {
        cout << "actual output//"
                " light: " << actual[i].getLight() <<
                " crime: " << actual[i].getCrime() <<
                " density: " << actual[i].getDensity() << " " <<
                " safety rating: " << actual[i].getSafetyRating() <<
                endl;
        cout << "expected output//"
                " light: " << expectedPath[i].getLight() <<
                " crime: " << expectedPath[i].getCrime() <<
                " density: " << expectedPath[i].getDensity() << " " <<
                " safety rating: " << expectedPath[i].getSafetyRating() <<
                endl;
    }
    */

