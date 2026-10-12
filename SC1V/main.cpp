// hi i don't have much confidence on this i do imagine a real vibe coder could produce a superior result then compared to mine.
#include <SFML/Graphics.hpp>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <iostream>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <utility>
#include <vector>

static const float PI = 3.14159265358979f;

// more ''helpers'' 
struct Vec3 { float x, y, z; };
static float dot(Vec3 a, Vec3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
static Vec3 cross(Vec3 a, Vec3 b) { return { a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x }; }
static Vec3 operator+(Vec3 a, Vec3 b) { return { a.x + b.x, a.y + b.y, a.z + b.z }; }
static Vec3 operator-(Vec3 a, Vec3 b) { return { a.x - b.x, a.y - b.y, a.z - b.z }; }
static Vec3 operator*(Vec3 a, float k) { return { a.x * k, a.y * k, a.z * k }; }
static float length(Vec3 a) { return std::sqrt(dot(a, a)); }
static Vec3 norm(Vec3 a) {
    float l = std::sqrt(dot(a, a));
    return l > 1e-6f ? Vec3{ a.x / l, a.y / l, a.z / l } : a;
}


struct Mesh {
    std::vector<Vec3> v;
    std::vector<std::vector<int>> f;
    std::vector<std::pair<int, int>> e;
};

static void finalize(Mesh& m) {
    std::set<std::pair<int, int>> s;
    for (auto& face : m.f)
        for (size_t i = 0; i < face.size(); ++i) {
            int a = face[i], b = face[(i + 1) % face.size()];
            if (a > b) std::swap(a, b);
            s.insert({ a, b });
        }
    m.e.assign(s.begin(), s.end());

    float r = 0.f;                                   // scale to unit radius
    for (auto& p : m.v) r = std::max(r, std::sqrt(dot(p, p)));
    if (r > 0.f) for (auto& p : m.v) { p.x /= r; p.y /= r; p.z /= r; }
}

static Mesh makeCube() {
    Mesh m;
    for (int i = 0; i < 8; ++i)
        m.v.push_back({ (i & 1) ? 1.f : -1.f, (i & 2) ? 1.f : -1.f, (i & 4) ? 1.f : -1.f });
    m.f = { {0,1,3,2}, {4,5,7,6}, {0,1,5,4}, {2,3,7,6}, {0,2,6,4}, {1,3,7,5} };
    finalize(m);
    return m;
}

static Mesh makeTetrahedron() {
    Mesh m;
    m.v = { {1,1,1}, {1,-1,-1}, {-1,1,-1}, {-1,-1,1} };
    m.f = { {0,1,2}, {0,1,3}, {0,2,3}, {1,2,3} };
    finalize(m);
    return m;
}

static Mesh makeOctahedron() {
    Mesh m;
    m.v = { {1,0,0}, {-1,0,0}, {0,1,0}, {0,-1,0}, {0,0,1}, {0,0,-1} };
    for (int a = 0; a < 2; ++a)
        for (int b = 2; b < 4; ++b)
            for (int c = 4; c < 6; ++c)
                m.f.push_back({ a, b, c });
    finalize(m);
    return m;
}

static Mesh makeIcosahedron() {
    Mesh m;
    const float t = (1.f + std::sqrt(5.f)) / 2.f;
    m.v = { {-1,t,0}, {1,t,0}, {-1,-t,0}, {1,-t,0},
            {0,-1,t}, {0,1,t}, {0,-1,-t}, {0,1,-t},
            {t,0,-1}, {t,0,1}, {-t,0,-1}, {-t,0,1} };
    m.f = { {0,11,5}, {0,5,1}, {0,1,7}, {0,7,10}, {0,10,11},
            {1,5,9}, {5,11,4}, {11,10,2}, {10,7,6}, {7,1,8},
            {3,9,4}, {3,4,2}, {3,2,6}, {3,6,8}, {3,8,9},
            {4,9,5}, {2,4,11}, {6,2,10}, {8,6,7}, {9,8,1} };
    finalize(m);
    return m;
}

// Dodecahedron = dual of the icosahedron
static Mesh makeDodecahedron() {
    Mesh ico = makeIcosahedron();
    Mesh m;
    for (auto& face : ico.f) {
        Vec3 c{ 0, 0, 0 };
        for (int i : face) { c.x += ico.v[i].x; c.y += ico.v[i].y; c.z += ico.v[i].z; }
        m.v.push_back(c);
    }
    for (size_t vi = 0; vi < ico.v.size(); ++vi) {
        std::vector<int> around;
        for (size_t fi = 0; fi < ico.f.size(); ++fi)
            for (int i : ico.f[fi])
                if (i == (int)vi) around.push_back((int)fi);

        Vec3 n = norm(ico.v[vi]);
        Vec3 helper = std::fabs(n.x) < 0.9f ? Vec3{ 1, 0, 0 } : Vec3{ 0, 1, 0 };
        Vec3 u = norm(cross(n, helper));
        Vec3 w = cross(n, u);
        auto ang = [&](int fi) { return std::atan2(dot(m.v[fi], w), dot(m.v[fi], u)); };
        std::sort(around.begin(), around.end(), [&](int a, int b) { return ang(a) < ang(b); });
        m.f.push_back(around);
    }
    finalize(m);
    return m;
}

static Mesh makeSphere(int stacks = 12, int slices = 20) {
    Mesh m;
    m.v.push_back({ 0, 1, 0 });
    for (int i = 1; i < stacks; ++i) {
        float phi = PI * i / stacks;
        for (int j = 0; j < slices; ++j) {
            float th = 2 * PI * j / slices;
            m.v.push_back({ std::sin(phi) * std::cos(th), std::cos(phi), std::sin(phi) * std::sin(th) });
        }
    }
    m.v.push_back({ 0, -1, 0 });
    int bottom = (int)m.v.size() - 1;
    auto idx = [&](int ring, int j) { return 1 + ring * slices + (j % slices); };
    for (int j = 0; j < slices; ++j) m.f.push_back({ 0, idx(0, j + 1), idx(0, j) });
    for (int r = 0; r < stacks - 2; ++r)
        for (int j = 0; j < slices; ++j)
            m.f.push_back({ idx(r, j), idx(r, j + 1), idx(r + 1, j + 1), idx(r + 1, j) });
    for (int j = 0; j < slices; ++j) m.f.push_back({ bottom, idx(stacks - 2, j), idx(stacks - 2, j + 1) });
    finalize(m);
    return m;
}

static Mesh makeTorus(int segs = 24, int rings = 12) {
    Mesh m;
    const float R = 0.7f, r = 0.3f;
    for (int i = 0; i < segs; ++i)
        for (int j = 0; j < rings; ++j) {
            float u = 2 * PI * i / segs, v = 2 * PI * j / rings;
            m.v.push_back({ (R + r * std::cos(v)) * std::cos(u), r * std::sin(v), (R + r * std::cos(v)) * std::sin(u) });
        }
    auto idx = [&](int i, int j) { return (i % segs) * rings + (j % rings); };
    for (int i = 0; i < segs; ++i)
        for (int j = 0; j < rings; ++j)
            m.f.push_back({ idx(i, j), idx(i + 1, j), idx(i + 1, j + 1), idx(i, j + 1) });
    finalize(m);
    return m;
}

static Mesh makePrism(int n) {
    Mesh m;
    for (int k = 0; k < 2; ++k)
        for (int i = 0; i < n; ++i) {
            float a = 2 * PI * i / n;
            m.v.push_back({ std::cos(a), k == 0 ? 0.8f : -0.8f, std::sin(a) });
        }
    std::vector<int> top, bot;
    for (int i = 0; i < n; ++i) {
        int j = (i + 1) % n;
        m.f.push_back({ i, j, n + j, n + i });
        top.push_back(i);
        bot.push_back(n + i);
    }
    m.f.push_back(top);
    m.f.push_back(bot);
    finalize(m);
    return m;
}

static Mesh makePyramid(int n) {
    Mesh m;
    std::vector<int> base;
    for (int i = 0; i < n; ++i) {
        float a = 2 * PI * i / n;
        m.v.push_back({ std::cos(a), -0.8f, std::sin(a) });
        base.push_back(i);
    }
    m.v.push_back({ 0, 1.0f, 0 });
    for (int i = 0; i < n; ++i) m.f.push_back({ i, (i + 1) % n, n });
    m.f.push_back(base);
    finalize(m);
    return m;
}

static Mesh makeBipyramid(int n) {
    Mesh m;
    for (int i = 0; i < n; ++i) {
        float a = 2 * PI * i / n;
        m.v.push_back({ std::cos(a), 0, std::sin(a) });
    }
    m.v.push_back({ 0, 1.1f, 0 });
    m.v.push_back({ 0, -1.1f, 0 });
    for (int i = 0; i < n; ++i) {
        int j = (i + 1) % n;
        m.f.push_back({ i, j, n });
        m.f.push_back({ i, j, n + 1 });
    }
    finalize(m);
    return m;
}

static Mesh makeAntiprism(int n) {
    Mesh m;
    for (int i = 0; i < n; ++i) {
        float a = 2 * PI * i / n;
        m.v.push_back({ std::cos(a), 0.5f, std::sin(a) });
    }
    for (int i = 0; i < n; ++i) {
        float a = 2 * PI * (i + 0.5f) / n;
        m.v.push_back({ std::cos(a), -0.5f, std::sin(a) });
    }
    std::vector<int> top, bot;
    for (int i = 0; i < n; ++i) {
        int j = (i + 1) % n;
        m.f.push_back({ i, j, n + i });
        m.f.push_back({ j, n + j, n + i });
        top.push_back(i);
        bot.push_back(n + i);
    }
    m.f.push_back(top);
    m.f.push_back(bot);
    finalize(m);
    return m;
}

// settings and modes and stuff
static const std::vector<std::string> kShapes = {
    "Cube", "Tetrahedron", "Octahedron", "Dodecahedron", "Icosahedron",
    "Sphere", "Torus", "Cylinder", "Cone",
    "Custom Prism", "Custom Pyramid", "Custom Bipyramid", "Custom Antiprism" };
static const int kFirstCustom = 9;

static const std::vector<std::string> kAxes = { "X + Y + Z", "Y only", "X only", "Z only", "X + Y" };
static const float kAxisW[5][3] = { {0.6f, 1.f, 0.3f}, {0, 1, 0}, {1, 0, 0}, {0, 0, 1}, {0.7f, 1, 0} };
static const std::vector<std::string> kRender = { "Wireframe", "Solid", "Solid + Outline" };
static const std::vector<std::string> kPatterns = { "Solid color", "Gradient", "Checkerboard", "Stripes", "Grid" };
static const std::vector<std::pair<std::string, sf::Color>> kTextColors = {
    { "White", sf::Color(255, 255, 255) }, { "Black", sf::Color(0, 0, 0) },
    { "Yellow", sf::Color(255, 230, 40) }, { "Red", sf::Color(255, 60, 60) },
    { "Lime", sf::Color(120, 255, 80) },   { "Cyan", sf::Color(60, 230, 255) },
    { "Magenta", sf::Color(255, 70, 230) }, { "orange", sf::Color(255, 150, 30) } };

struct Settings {
    int   shape = 0;
    int   sides = 5;          // used by the custom shapes
    float speed = 1.0f;       // radians per sek.
    int   axes = 0;
    int   render = 2;
    int   hue = 200;
    bool  textOn = true;
    sf::String text = "Hello!";
    sf::String editBackup;
    int   textColor = 0;
    bool  editing = false;
    bool  textDirty = true;
    int   bgPattern = 1;
    int   c1[3] = { 15, 18, 40 };
    int   c2[3] = { 55, 25, 95 };
    int   patSize = 60;
    float zoom = 1.0f;
    bool  fullscreen = false;
    bool  paused = false;
    bool  menu = true;
    bool  meshDirty = true;
};

enum Item {
    I_SHAPE, I_SIDES, I_SPEED, I_AXES, I_RENDER, I_HUE,
    I_TEXTON, I_TEXT, I_TEXTCOL,
    I_BGPAT, I_C1R, I_C1G, I_C1B, I_C2R, I_C2G, I_C2B, I_PATSIZE,
    I_FULL, I_COUNT
};

static Mesh buildMesh(const Settings& s) {
    switch (s.shape) {
    case 0: return makeCube();
    case 1: return makeTetrahedron();
    case 2: return makeOctahedron();
    case 3: return makeDodecahedron();
    case 4: return makeIcosahedron();
    case 5: return makeSphere();
    case 6: return makeTorus();
    case 7: return makePrism(32);
    case 8: return makePyramid(32);
    case 9: return makePrism(s.sides);
    case 10: return makePyramid(s.sides);
    case 11: return makeBipyramid(s.sides);
    default: return makeAntiprism(s.sides);
    }
}

static int wrap(int v, int n) { return ((v % n) + n) % n; }

static void adjust(Settings& s, int item, int dir, bool big, bool& toggleFullscreen) {
    auto clampi = [](int v, int lo, int hi) { return std::max(lo, std::min(hi, v)); };
    int step = big ? 5 : 1;
    switch (item) {
    case I_SHAPE:  s.shape = wrap(s.shape + dir, (int)kShapes.size()); s.meshDirty = true; break;
    case I_SIDES:  s.sides = clampi(s.sides + dir * step, 3, 64);      s.meshDirty = true; break;
    case I_SPEED:  s.speed = std::max(0.f, std::min(20.f, s.speed + dir * (big ? 1.0f : 0.1f))); break;
    case I_AXES:   s.axes = wrap(s.axes + dir, (int)kAxes.size()); break;
    case I_RENDER: s.render = wrap(s.render + dir, (int)kRender.size()); break;
    case I_HUE:    s.hue = wrap(s.hue + dir * (big ? 30 : 5), 360); break;
    case I_TEXTON: s.textOn = !s.textOn; break;
    case I_TEXTCOL: s.textColor = wrap(s.textColor + dir, (int)kTextColors.size()); break;
    case I_BGPAT:  s.bgPattern = wrap(s.bgPattern + dir, (int)kPatterns.size()); break;
    case I_C1R: case I_C1G: case I_C1B: {
        int& c = s.c1[item - I_C1R]; c = clampi(c + dir * (big ? 25 : 5), 0, 255); break;
    }
    case I_C2R: case I_C2G: case I_C2B: {
        int& c = s.c2[item - I_C2R]; c = clampi(c + dir * (big ? 25 : 5), 0, 255); break;
    }
    case I_PATSIZE: s.patSize = clampi(s.patSize + dir * (big ? 20 : 5), 10, 300); break;
    case I_FULL:    toggleFullscreen = true; break;
    }
}


// Drawing ''helpers'' or whatever claude ai or stackoverflow

static sf::Color hsv(float h, float s, float v) {
    float c = v * s;
    float hp = std::fmod(h, 360.f) / 60.f;
    float x = c * (1.f - std::fabs(std::fmod(hp, 2.f) - 1.f));
    float r = 0, g = 0, b = 0;
    if (hp < 1) { r = c; g = x; }
    else if (hp < 2) { r = x; g = c; }
    else if (hp < 3) { g = c; b = x; }
    else if (hp < 4) { g = x; b = c; }
    else if (hp < 5) { r = x; b = c; }
    else { r = c; b = x; }
    float m = v - c;
    return sf::Color((std::uint8_t)((r + m) * 255), (std::uint8_t)((g + m) * 255), (std::uint8_t)((b + m) * 255));
}

typedef sf::Vector2f V2;

static void addTri(sf::VertexArray& va, V2 a, V2 b, V2 c, sf::Color col) {
    va.append(sf::Vertex{ a, col });
    va.append(sf::Vertex{ b, col });
    va.append(sf::Vertex{ c, col });
}
static void addQuad(sf::VertexArray& va, V2 a, V2 b, V2 c, V2 d, sf::Color col) {
    addTri(va, a, b, c, col);
    addTri(va, a, c, d, col);
}
static void addLine(sf::VertexArray& va, V2 a, V2 b, float thick, sf::Color col) {
    V2 d = b - a;
    float len = std::sqrt(d.x * d.x + d.y * d.y);
    if (len < 1e-3f) return;
    V2 n(-d.y / len * thick * 0.5f, d.x / len * thick * 0.5f);
    addQuad(va, a + n, a - n, b - n, b + n, col);
}

static void drawBackground(sf::RenderWindow& win, const Settings& s) {
    sf::Color c1((std::uint8_t)s.c1[0], (std::uint8_t)s.c1[1], (std::uint8_t)s.c1[2]);
    sf::Color c2((std::uint8_t)s.c2[0], (std::uint8_t)s.c2[1], (std::uint8_t)s.c2[2]);
    float W = (float)win.getSize().x, H = (float)win.getSize().y;
    float sz = (float)s.patSize;
    win.clear(c1);

    sf::VertexArray va(sf::PrimitiveType::Triangles);
    switch (s.bgPattern) {
    case 1: // vertical gradient
        va.append(sf::Vertex{ V2(0, 0), c1 }); va.append(sf::Vertex{ V2(W, 0), c1 }); va.append(sf::Vertex{ V2(W, H), c2 });
        va.append(sf::Vertex{ V2(0, 0), c1 }); va.append(sf::Vertex{ V2(W, H), c2 }); va.append(sf::Vertex{ V2(0, H), c2 });
        break;
    case 2: // checkerboard aka godot background
        for (int j = 0; j * sz < H; ++j)
            for (int i = 0; i * sz < W; ++i)
                if ((i + j) & 1)
                    addQuad(va, V2(i * sz, j * sz), V2((i + 1) * sz, j * sz), V2((i + 1) * sz, (j + 1) * sz), V2(i * sz, (j + 1) * sz), c2);
        break;
    case 3: // diagonal stripes
        for (float x = 0; x < W + H; x += sz * 2)
            addQuad(va, V2(x, 0), V2(x + sz, 0), V2(x + sz - H, H), V2(x - H, H), c2);
        break;
    case 4: // grid
        for (float x = 0; x < W; x += sz) addLine(va, V2(x, 0), V2(x, H), 2.f, c2);
        for (float y = 0; y < H; y += sz) addLine(va, V2(0, y), V2(W, y), 2.f, c2);
        break;
    default: break;
    }
    if (va.getVertexCount()) win.draw(va);
}

// text on shape feature... very cool
struct Decal {
    bool  ok = false;          // does this shape have a flat face big enough for text?
    int   face = -1;
    Vec3  c{ 0, 0, 0 }, n{ 0, 0, 1 }, u{ 1, 0, 0 }, v{ 0, 1, 0 };   
    float halfW = 0.f, halfH = 0.f;                                   
};

struct TextTexture {
    std::unique_ptr<sf::RenderTexture> rt;
    float aspect = 1.f;
};

// Wrap long text into several lines so it fills a face better.
static sf::String wrapText(const sf::String& str) {
    std::u32string src = str.toUtf32();
    size_t maxChars = std::max<size_t>(8, (size_t)std::ceil(std::sqrt(src.size() * 3.0)));
    std::u32string out, line, word;
    auto flushWord = [&]() {
        if (word.empty()) return;
        if (!line.empty() && line.size() + 1 + word.size() > maxChars) { out += line; out += U'\n'; line.clear(); }
        if (!line.empty()) line += U' ';
        line += word;
        word.clear();
        };
    for (char32_t ch : src) { if (ch == U' ') flushWord(); else word += ch; }
    flushWord();
    out += line;
    return sf::String(out);
}

static bool buildTextTexture(TextTexture& tt, const sf::Font& font, const sf::String& str) {
    sf::Text t(font, wrapText(str), 96);
    t.setFillColor(sf::Color::White);
    sf::FloatRect b = t.getLocalBounds();
    float pad = 10.f;
    float w = b.size.x + 2 * pad, h = b.size.y + 2 * pad;
    float k = w > 3000.f ? 3000.f / w : 1.f;
    unsigned tw = std::max(8u, (unsigned)std::ceil(w * k)), th = std::max(8u, (unsigned)std::ceil(h * k));
    t.setOrigin(b.position);
    t.setPosition({ pad * k, pad * k });
    t.setScale({ k, k });

    auto rt = std::make_unique<sf::RenderTexture>();
    if (!rt->resize({ tw, th })) return false;
    rt->setSmooth(true);
    rt->clear(sf::Color::Transparent);
    rt->draw(t);
    rt->display();
    tt.rt = std::move(rt);
    tt.aspect = (float)tw / (float)th;
    return true;
}

// Find the largest perfectly flat face and the biggest text rectangle.
static Decal findDecal(const Mesh& m, float aspect) {
    Decal d;
    float bestArea = 0.f;
    for (size_t fi = 0; fi < m.f.size(); ++fi) {
        const auto& f = m.f[fi];
        if (f.size() < 3) continue;
        Vec3 nn{ 0, 0, 0 }, c{ 0, 0, 0 };
        for (size_t k = 0; k < f.size(); ++k) {
            Vec3 a = m.v[f[k]], b = m.v[f[(k + 1) % f.size()]];
            nn.x += (a.y - b.y) * (a.z + b.z);
            nn.y += (a.z - b.z) * (a.x + b.x);
            nn.z += (a.x - b.x) * (a.y + b.y);
            c = c + a;
        }
        c = c * (1.f / f.size());
        float area = 0.5f * length(nn);
        if (area < 1e-5f) continue;
        Vec3 n = norm(nn);
        float dev = 0.f;                                   // planarity check
        for (int i : f) dev = std::max(dev, std::fabs(dot(m.v[i] - c, n)));
        if (dev > 0.01f) continue;
        if (area > bestArea + 1e-4f) {
            bestArea = area;
            d.face = (int)fi;
            d.c = c;
            d.n = dot(n, c) < 0.f ? n * -1.f : n;          // make it point outwards
        }
    }
    // Uneven shapes i didn't bother trying to think of a solution. since one doesn't exist!!!!!!11!
    if (d.face < 0 || bestArea < 0.15f) return Decal{};

    const auto& f = m.f[d.face];
    float bestLen = 0.f;
    Vec3 u{ 1, 0, 0 };
    for (size_t k = 0; k < f.size(); ++k) {                // text runs along the longest edge
        Vec3 e = m.v[f[(k + 1) % f.size()]] - m.v[f[k]];
        float l = length(e);
        if (l > bestLen) { bestLen = l; u = e * (1.f / l); }
    }
    u = norm(u - d.n * dot(u, d.n));
    d.u = u;
    d.v = cross(u, d.n);                                  

    float t = 1e9f;                                        
    for (size_t k = 0; k < f.size(); ++k) {
        Vec3 p = m.v[f[k]], q = m.v[f[(k + 1) % f.size()]];
        Vec3 inward = norm(cross(d.n, q - p));
        float dist = dot(inward, d.c - p);
        if (dist < 0.f) { inward = inward * -1.f; dist = -dist; }
        float denom = std::fabs(dot(inward, d.u)) * aspect + std::fabs(dot(inward, d.v));
        if (denom > 1e-6f && dist > 1e-6f) t = std::min(t, dist / denom);
    }
    d.halfW = t * aspect * 0.85f;
    d.halfH = t * 0.85f;
    d.ok = true;
    return d;
}

static void drawShape(sf::RenderWindow& win, const Mesh& m, float ax, float ay, float az, const Settings& s,
    const Decal& decal, const sf::Texture* tex) {
    const float sx = std::sin(ax), cx = std::cos(ax);
    const float sy = std::sin(ay), cy = std::cos(ay);
    const float sz = std::sin(az), cz = std::cos(az);
    const float D = 4.f;
    float W = (float)win.getSize().x, H = (float)win.getSize().y;
    float scale = std::min(W, H) * 0.35f * s.zoom;

    auto rotate = [&](Vec3 v) {
        float y1 = v.y * cx - v.z * sx, z1 = v.y * sx + v.z * cx, x1 = v.x;   // XYZ axis
        float x2 = x1 * cy + z1 * sy, z2 = -x1 * sy + z1 * cy;                 
        float x3 = x2 * cz - y1 * sz, y3 = x2 * sz + y1 * cz;                  
        return Vec3{ x3, y3, z2 };
        };
    auto project = [&](Vec3 r) {
        float k = D / (D + r.z);
        return V2(W * 0.5f + r.x * scale * k, H * 0.5f - r.y * scale * k);
        };

    std::vector<Vec3> r(m.v.size());
    std::vector<V2> p(m.v.size());
    for (size_t i = 0; i < m.v.size(); ++i) { r[i] = rotate(m.v[i]); p[i] = project(r[i]); }

    sf::Color base = hsv((float)s.hue, 0.75f, 1.f);

    // it should appear on the flattest space and put it in front of the ''camera''
    bool showDecal = tex && decal.ok && s.textOn;
    bool decalFront = false;
    if (showDecal) {
        Vec3 dn = rotate(decal.n), dc = rotate(decal.c);
        decalFront = dot(dn, Vec3{ 0, 0, -D } - dc) > 0.f;
    }
    auto drawDecal = [&](float shade) {
        if (!showDecal || !decalFront) return;
        const int N = 10;                                  // subdivide so pov is correct0
        sf::Color tc = kTextColors[s.textColor].second;
        sf::Color col((std::uint8_t)(tc.r * shade), (std::uint8_t)(tc.g * shade), (std::uint8_t)(tc.b * shade));
        sf::Vector2u ts = tex->getSize();
        auto vert = [&](int i, int j) {
            float x = -1.f + 2.f * i / N, y = -1.f + 2.f * j / N;
            Vec3 pt = rotate(decal.c + decal.u * (x * decal.halfW) + decal.v * (y * decal.halfH));
            return sf::Vertex{ project(pt), col, V2((x + 1.f) * 0.5f * ts.x, (1.f - y) * 0.5f * ts.y) };
            };
        sf::VertexArray dv(sf::PrimitiveType::Triangles);
        for (int j = 0; j < N; ++j)
            for (int i = 0; i < N; ++i) {
                sf::Vertex a = vert(i, j), b = vert(i + 1, j), c = vert(i + 1, j + 1), d = vert(i, j + 1);
                dv.append(a); dv.append(b); dv.append(c);
                dv.append(a); dv.append(c); dv.append(d);
            }
        sf::RenderStates rs;
        rs.texture = tex;
        win.draw(dv, rs);
        };

    sf::VertexArray va(sf::PrimitiveType::Triangles);

    if (s.render == 0) {                                  // wireframe
        for (auto& e : m.e) addLine(va, p[e.first], p[e.second], 2.5f, base);
        win.draw(va);
        drawDecal(1.f);
    }
    else {                                                // i have no idea how painters algorithm works
        struct FD { float depth; int idx; };
        std::vector<FD> order;
        for (size_t i = 0; i < m.f.size(); ++i) {
            float d = 0;
            for (int k : m.f[i]) d += r[k].z;
            order.push_back({ d / m.f[i].size(), (int)i });
        }
        std::sort(order.begin(), order.end(), [](const FD& a, const FD& b) { return a.depth > b.depth; });

        const Vec3 L = norm({ -0.4f, 0.6f, -0.7f });
        for (auto& fd : order) {
            const auto& f = m.f[fd.idx];
            Vec3 n{ 0, 0, 0 };                            // Newell normal
            for (size_t k = 0; k < f.size(); ++k) {
                Vec3 a = r[f[k]], b = r[f[(k + 1) % f.size()]];
                n.x += (a.y - b.y) * (a.z + b.z);
                n.y += (a.z - b.z) * (a.x + b.x);
                n.z += (a.x - b.x) * (a.y + b.y);
            }
            n = norm(n);
            float shade = 0.25f + 0.75f * std::fabs(dot(n, L));
            sf::Color col((std::uint8_t)(base.r * shade), (std::uint8_t)(base.g * shade), (std::uint8_t)(base.b * shade));
            for (size_t k = 1; k + 1 < f.size(); ++k) addTri(va, p[f[0]], p[f[k]], p[f[k + 1]], col);

            if (fd.idx == decal.face && showDecal && decalFront) {   // draw the text right on top of its face
                win.draw(va);
                va.clear();
                drawDecal(shade);
            }
            if (s.render == 2)
                for (size_t k = 0; k < f.size(); ++k)
                    addLine(va, p[f[k]], p[f[(k + 1) % f.size()]], 1.5f, sf::Color(0, 0, 0, 170));
        }
        win.draw(va);
    }
}

static void createWindow(sf::RenderWindow& w, bool full) {
    sf::ContextSettings cs;
    cs.antiAliasingLevel = 4;
    if (full) w.create(sf::VideoMode::getDesktopMode(), "Spinning Shapes", sf::Style::Default, sf::State::Fullscreen, cs);
    else      w.create(sf::VideoMode({ 1280u, 720u }), "Spinning Shapes", sf::Style::Default, sf::State::Windowed, cs);
    w.setVerticalSyncEnabled(true);
    w.setView(sf::View(sf::FloatRect({ 0.f, 0.f }, { (float)w.getSize().x, (float)w.getSize().y })));
}


// hud / menu

static const float kPanelX = 12, kPanelY = 12, kPanelW = 400, kHeaderH = 44, kRowH = 28;

static std::vector<std::pair<std::string, sf::String>> menuRows(const Settings& s, bool textOk) {
    char buf[32];
    std::vector<std::pair<std::string, sf::String>> rows(I_COUNT);
    rows[I_SHAPE] = { "Shape", kShapes[s.shape] };
    rows[I_SIDES] = { s.shape >= kFirstCustom ? "Sides" : "Sides (custom shapes only)", std::to_string(s.sides) };
    std::snprintf(buf, sizeof buf, "%.1f", s.speed);
    rows[I_SPEED] = { "Spin speed", buf };
    rows[I_AXES] = { "Spin axes", kAxes[s.axes] };
    rows[I_RENDER] = { "Render", kRender[s.render] };
    rows[I_HUE] = { "Shape color (hue)", std::to_string(s.hue) };

    rows[I_TEXTON] = { "Text on flat face", textOk ? (s.textOn ? "On" : "Off") : "n/a" };
    sf::String shown = s.text;
    if (s.editing) shown += sf::String("_");
    if (shown.getSize() > 16) shown = sf::String("...") + shown.substring(shown.getSize() - 13);
    rows[I_TEXT] = { "Text  (Enter = edit)", textOk ? shown : sf::String("(no flat face)") };
    rows[I_TEXTCOL] = { "Text color", kTextColors[s.textColor].first };
        

    // this sucks
    rows[I_BGPAT] = { "Background", kPatterns[s.bgPattern] };
    rows[I_C1R] = { "Background color 1 - R", std::to_string(s.c1[0]) };
    rows[I_C1G] = { "Background color 1 - G", std::to_string(s.c1[1]) };
    rows[I_C1B] = { "Background color 1 - B", std::to_string(s.c1[2]) };
    rows[I_C2R] = { "Background color 2 - R", std::to_string(s.c2[0]) };
    rows[I_C2G] = { "Background color 2 - G", std::to_string(s.c2[1]) };
    rows[I_C2B] = { "Background color 2 - B", std::to_string(s.c2[2]) };
    rows[I_PATSIZE] = { "Pattern size", std::to_string(s.patSize) };
    rows[I_FULL] = { "Fullscreen (F11)", s.fullscreen ? "On" : "Off" };
    return rows;
}

static void drawMenu(sf::RenderWindow& win, const sf::Font& font, const Settings& s, int selected, bool textOk) {
    auto rows = menuRows(s, textOk);
    float panelH = kHeaderH + rows.size() * kRowH + 30;

    sf::RectangleShape panel(V2(kPanelW, panelH));
    panel.setPosition({ kPanelX, kPanelY });
    panel.setFillColor(sf::Color(0, 0, 0, 175));
    win.draw(panel);

    sf::Text t(font, "", 18);
    t.setString(s.editing ? "TYPING...   Enter = OK,  Esc = cancel" : "SPINNING SHAPES   [Tab] hide menu");
    t.setPosition({ kPanelX + 12, kPanelY + 10 });
    t.setFillColor(s.editing ? sf::Color(255, 230, 40) : sf::Color(255, 255, 255));
    win.draw(t);

    for (int i = 0; i < (int)rows.size(); ++i) {
        float y = kPanelY + kHeaderH + i * kRowH;
        if (i == selected) {
            sf::RectangleShape hi(V2(kPanelW - 12, kRowH - 2));
            hi.setPosition({ kPanelX + 6, y });
            hi.setFillColor(sf::Color(90, 130, 255, 120));
            win.draw(hi);
        }
        bool dim = (i == I_SIDES && s.shape < kFirstCustom) ||
            (!textOk && (i == I_TEXTON || i == I_TEXT || i == I_TEXTCOL));
        t.setFillColor(dim ? sf::Color(130, 130, 130) : sf::Color::White);
        t.setCharacterSize(16);
        t.setString(rows[i].first);
        t.setPosition({ kPanelX + 14, y + 3 });
        win.draw(t);

        sf::String val = (i == I_TEXT) ? sf::String("[ ") + rows[i].second + sf::String(" ]")
            : sf::String("<  ") + rows[i].second + sf::String("  >");
        t.setString(val);
        t.setPosition({ kPanelX + kPanelW * 0.55f, y + 3 });
        win.draw(t);
    }

    t.setCharacterSize(13);
    t.setFillColor(sf::Color(170, 170, 170));
    t.setString("Space: pause   Wheel: zoom   F11: fullscreen");
    t.setPosition({ kPanelX + 14, kPanelY + kHeaderH + rows.size() * kRowH + 4 });
    win.draw(t);
}

// hmm you could guess what ts do
int main() {
    sf::Font font;
    const char* fontPaths[] = { "font.ttf", "C:/Windows/Fonts/segoeui.ttf", "C:/Windows/Fonts/arial.ttf" };
    bool fontOk = false;
    for (auto path : fontPaths) if (font.openFromFile(path)) { fontOk = true; break; }
    if (!fontOk) {
        std::cerr << "Could not load a font. Put any .ttf file named font.ttf next to the .exe.\n";
        return 1;
    }

    Settings s;
    sf::RenderWindow window;
    createWindow(window, false);

    Mesh mesh = buildMesh(s);
    Decal decal;
    TextTexture textTex;
    bool texReady = false;
    float ax = 0.4f, ay = 0.f, az = 0.f;
    int selected = 0;
    sf::Clock clock;

    while (window.isOpen()) {
        bool toggleFs = false;
        while (const std::optional<sf::Event> ev = window.pollEvent()) {
            if (ev->is<sf::Event::Closed>()) {
                window.close();
            }
            else if (const auto* rs = ev->getIf<sf::Event::Resized>()) {
                window.setView(sf::View(sf::FloatRect({ 0.f, 0.f }, { (float)rs->size.x, (float)rs->size.y })));
            }
            else if (const auto* key = ev->getIf<sf::Event::KeyPressed>()) {
                if (s.editing) {                                   // typing text: only Enter / Esc matter here
                    if (key->code == sf::Keyboard::Key::Escape) { s.text = s.editBackup; s.editing = false; s.textDirty = true; }
                    else if (key->code == sf::Keyboard::Key::Enter) { s.editing = false; s.textDirty = true; }
                }
                else switch (key->code) {
                case sf::Keyboard::Key::Escape:
                    if (s.fullscreen) toggleFs = true; else window.close();
                    break;
                case sf::Keyboard::Key::Tab: case sf::Keyboard::Key::M: s.menu = !s.menu; break;
                case sf::Keyboard::Key::Space: s.paused = !s.paused; break;
                case sf::Keyboard::Key::F11: toggleFs = true; break;
                case sf::Keyboard::Key::Enter:
                    if (s.menu && selected == I_TEXT && decal.ok) { s.editing = true; s.editBackup = s.text; }
                    break;
                case sf::Keyboard::Key::Up:   if (s.menu) selected = wrap(selected - 1, I_COUNT); break;
                case sf::Keyboard::Key::Down: if (s.menu) selected = wrap(selected + 1, I_COUNT); break;
                case sf::Keyboard::Key::Left:  if (s.menu) adjust(s, selected, -1, key->shift, toggleFs); break;
                case sf::Keyboard::Key::Right: if (s.menu) adjust(s, selected, +1, key->shift, toggleFs); break;
                default: break;
                }
            }
            else if (const auto* te = ev->getIf<sf::Event::TextEntered>()) {
                if (s.editing) {
                    char32_t ch = te->unicode;
                    if (ch == 8) { if (s.text.getSize() > 0) s.text.erase(s.text.getSize() - 1); s.textDirty = true; }
                    else if (ch >= 32 && ch != 127 && s.text.getSize() < 80) { s.text += sf::String(ch); s.textDirty = true; }
                }
            }
            else if (const auto* wheel = ev->getIf<sf::Event::MouseWheelScrolled>()) {
                s.zoom *= (1.f + 0.1f * wheel->delta);
                s.zoom = std::max(0.3f, std::min(3.f, s.zoom));
            }
            else if (const auto* click = ev->getIf<sf::Event::MouseButtonPressed>()) {
                if (click->button == sf::Mouse::Button::Left && s.menu && !s.editing) {
                    float mx = (float)click->position.x, my = (float)click->position.y;
                    float top = kPanelY + kHeaderH;
                    if (mx >= kPanelX && mx <= kPanelX + kPanelW && my >= top && my < top + I_COUNT * kRowH) {
                        selected = (int)((my - top) / kRowH);
                        if (selected == I_TEXT) {
                            if (mx > kPanelX + kPanelW * 0.55f && decal.ok) { s.editing = true; s.editBackup = s.text; }
                        }
                        else if (mx > kPanelX + kPanelW * 0.55f)
                            adjust(s, selected, mx < kPanelX + kPanelW * 0.78f ? -1 : +1, false, toggleFs);
                    }
                }
            }
        }

        if (toggleFs) {
            s.fullscreen = !s.fullscreen;
            createWindow(window, s.fullscreen);
        }
        if (s.meshDirty) { mesh = buildMesh(s); s.meshDirty = false; s.textDirty = true; }
        if (s.textDirty) {
            s.textDirty = false;
            texReady = !s.text.isEmpty() && buildTextTexture(textTex, font, s.text);
            decal = findDecal(mesh, texReady ? textTex.aspect : 1.f);
        }

        float dt = clock.restart().asSeconds();
        if (!s.paused) {
            ax += s.speed * kAxisW[s.axes][0] * dt;
            ay += s.speed * kAxisW[s.axes][1] * dt;
            az += s.speed * kAxisW[s.axes][2] * dt;
        }

        drawBackground(window, s);
        drawShape(window, mesh, ax, ay, az, s, decal, texReady ? &textTex.rt->getTexture() : nullptr);
        if (s.menu) drawMenu(window, font, s, selected, decal.ok);
        else {
            sf::Text hint(font, "Tab: menu", 14);
            hint.setPosition({ 10.f, 8.f });
            hint.setFillColor(sf::Color(255, 255, 255, 150));
            window.draw(hint);
        }
        window.display();
    }
    return 0;
}
