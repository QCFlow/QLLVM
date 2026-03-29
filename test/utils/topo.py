def generate_fully_connected_topology(n):
    edges = []
    for i in range(n):
        for j in range(i + 1, n):
            edges.append([i, j])
    return edges