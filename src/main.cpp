#include "geometrycentral/combinatorial-maps/combinatorial_map.h"
#include "geometrycentral/utilities/vector3.h"

#include "polyscope/polyscope.h"
#include "polyscope/surface_mesh.h"
#include "polyscope/volume_mesh.h"

#include "args/args.hxx"
#include "imgui.h"

using namespace geometrycentral;
using namespace geometrycentral::combinatorial_map;

using TetMesh = CombinatorialMap<3>;

template <size_t D, typename T>
using TetData = CellData<3, D, T>;

template <size_t D>
using Tet = Cell<3, D>;

using std::cout;
using std::endl;
using std::string;

// Polyscope visualization handle, to quickly add data to the surface
polyscope::VolumeMesh* psMesh;

// A user-defined callback, for creating control panels (etc)
// Use ImGUI commands to build whatever you want here, see
// https://github.com/ocornut/imgui/blob/master/imgui.h
void myCallback() {}

int main(int argc, char** argv) {

    // Configure the argument parser
    args::ArgumentParser parser("Combinatorial map visualizer");
    args::Positional<std::string> inputFilename(parser, "mesh",
                                                "Mesh to be processed.");

    // Parse args
    try {
        parser.ParseCLI(argc, argv);
    } catch (args::Help) {
        std::cout << parser;
        return 0;
    } catch (args::ParseError e) {
        std::cerr << e.what() << std::endl;
        std::cerr << parser;
        return 1;
    }

    std::string tetFilename = "../data/bunny_small.1.ele";
    // Make sure a mesh name was given
    if (inputFilename) {
        tetFilename = args::get(inputFilename);
    }

    std::vector<Vector3> tetVertices;
    std::vector<std::array<size_t, 4>> tets;

    string elePath =
        tetFilename.substr(0, tetFilename.find_last_of(".")) + ".ele";
    string nodePath =
        tetFilename.substr(0, tetFilename.find_last_of(".")) + ".node";

    std::ifstream node(nodePath);
    if (node.is_open()) {
        string line;
        getline(node, line);
        while (getline(node, line)) {
            if (line[0] == '#' || line.length() == 0) continue;

            std::istringstream ss(line);
            size_t idx;
            double x, y, z;
            ss >> idx >> x >> y >> z;
            tetVertices.emplace_back(Vector3{x, y, z});
            assert(idx == verts.size());
        }

        node.close();
    }

    std::ifstream ele(elePath);
    if (ele.is_open()) {
        string line;
        getline(ele, line);

        while (getline(ele, line)) {
            if (line[0] == '#' || line.length() == 0) continue;

            std::istringstream ss(line);
            size_t idx, a, b, c, d;
            ss >> idx >> a >> b >> c >> d;
            // subtract 1 since tetgen is 1-indexed
            // reorder vertices since tetgen uses the opposite orientation
            // convention
            tets.emplace_back(
                std::array<size_t, 4>{b - 1, a - 1, c - 1, d - 1});
        }

        ele.close();
    }

    TetMesh tetMesh(tets); // construct tet mesh
    VertexData<3, Vector3> vertexPositions(
        tetMesh); // copy vertex positions into a VertexData
    for (size_t i = 0; i < tetMesh.nVertices(); i++)
        vertexPositions[i] = tetVertices[i];

    // Initialize polyscope
    polyscope::init();

    // Set the callback function
    polyscope::state::userCallback = myCallback;

    psMesh = polyscope::registerTetMesh("bunny", vertexPositions,
                                        tetMesh.getCellVertexList<3>());

    // Define and register a vertex quantity
    VertexData<3, double> f(tetMesh);
    for (Vertex<3> i : tetMesh.vertices()) {
        f[i] = vertexPositions[i].x * sin(vertexPositions[i].y);
    }
    psMesh->addVertexScalarQuantity("vertex quantity", f);

    std::cout << "# 3-cells: " << tetMesh.nCells<3>() << std::endl;
    std::cout << "3-cell capacity: " << tetMesh.nCellsCapacity<3>()
              << std::endl;
    std::cout << "tet list length: " << tets.size() << std::endl;

    // Define and register a tet quantity
    TetData<3, double> g(tetMesh);
    for (Tet<3> t : tetMesh.cells<3>()) {
        Vector3 center = Vector3::zero();
        for (Vertex<3> i : t.adjacentVertices()) {
            center += vertexPositions[i];
        }
        center /= 4;
        g[t] = center.z;
    }
    psMesh->addCellScalarQuantity("tet quantity", g);


    // Add a slice plane
    polyscope::SlicePlane* psPlane = polyscope::addSceneSlicePlane();
    psPlane->setPose(glm::vec3{-1., 0., 0.}, glm::vec3{0.25, 0., -1.});
    psPlane->setDrawPlane(false); // render the semi-transparent gridded plane
    psPlane->setDrawWidget(true);

    // Give control to the polyscope gui
    polyscope::show();

    return EXIT_SUCCESS;
}
