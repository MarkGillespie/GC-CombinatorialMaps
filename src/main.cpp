#include "geometrycentral/combinatorial-maps/combinatorial_map.h"
#include "geometrycentral/surface/halfedge_mesh.h"
#include "geometrycentral/surface/meshio.h"
#include "geometrycentral/surface/vertex_position_geometry.h"

#include "polyscope/polyscope.h"
#include "polyscope/surface_mesh.h"
#include "polyscope/tet_mesh.h"

#include "args/args.hxx"
#include "imgui.h"

using namespace geometrycentral;
using namespace geometrycentral::surface;

using std::cout;
using std::endl;
using std::string;

// == Geometry-central data
std::unique_ptr<HalfedgeMesh> mesh;
std::unique_ptr<VertexPositionGeometry> geometry;

// Polyscope visualization handle, to quickly add data to the surface
polyscope::SurfaceMesh* psMesh;

// A user-defined callback, for creating control panels (etc)
// Use ImGUI commands to build whatever you want here, see
// https://github.com/ocornut/imgui/blob/master/imgui.h
void myCallback() {}

int main(int argc, char** argv) {

    // Configure the argument parser
    args::ArgumentParser parser("Geometry program");
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

    std::string filename    = "../../meshes/bunny_small.obj";
    std::string tetFilename = "../../meshes/TetMeshes/bunny_small.1.ele";
    // Make sure a mesh name was given
    if (inputFilename) {
        filename = args::get(inputFilename);
    }

    std::vector<Vector3> tetVertices;
    std::vector<std::vector<size_t>> tets;

    string elePath =
        tetFilename.substr(0, tetFilename.find_last_of(".")) + ".ele";
    string nodePath =
        tetFilename.substr(0, tetFilename.find_last_of(".")) + ".node";
    string neighPath =
        tetFilename.substr(0, tetFilename.find_last_of(".")) + ".neigh";


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
            tets.emplace_back(std::vector<size_t>{b - 1, a - 1, c - 1, d - 1});
        }

        ele.close();
    }

    combinatorial_map::CombinatorialMap<3> tm(tets);

    // Initialize polyscope
    polyscope::init();

    // Set the callback function
    polyscope::state::userCallback = myCallback;

    polyscope::registerTetMesh("bunny", tetVertices, tm.getCellVertexList<3>());

    // Load mesh
    std::tie(mesh, geometry) = loadMesh(filename);
    std::cout << "Genus: " << mesh->genus() << std::endl;

    combinatorial_map::CombinatorialMap<2> cm(mesh->getFaceVertexList());

    // // Register the mesh with polyscope
    psMesh = polyscope::registerSurfaceMesh(
        "bunny surface mesh", geometry->inputVertexPositions,
        cm.getCellVertexList<2>(), polyscopePermutations(*mesh));

    // Give control to the polyscope gui
    polyscope::show();

    return EXIT_SUCCESS;
}
