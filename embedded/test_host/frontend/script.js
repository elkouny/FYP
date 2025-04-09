const boardEl = document.getElementById("board");
let selected = null;
let validMoves = [];
const pieceUnicode = {
  P: "♙",
  N: "♘",
  B: "♗",
  R: "♖",
  Q: "♕",
  K: "♔",
  p: "♟",
  n: "♞",
  b: "♝",
  r: "♜",
  q: "♛",
  k: "♚",
};

function createSquare(x, y) {
  const square = document.createElement("div");
  square.classList.add("square", (x + y) % 2 === 0 ? "light" : "dark");
  square.dataset.x = x + 1;
  square.dataset.y = y + 1;
  square.addEventListener("click", handleClick);
  return square;
}

function handleClick(e) {
  const x = parseInt(e.target.dataset.x);
  const y = parseInt(e.target.dataset.y);

  if (selected && validMoves.some((m) => m.x === x && m.y === y)) {
    fetch(
      `http://localhost:8080/move_piece?fromX=${selected.x}&fromY=${selected.y}&toX=${x}&toY=${y}`
    ).then(() => {
      selected = null;
      validMoves = [];
      renderBoard();
    });
  } else {
    fetch(`http://localhost:8080/valid_moves?x=${x}&y=${y}`)
      .then((res) => res.json())
      .then((moves) => {
        selected = { x, y };
        validMoves = moves;
        renderBoard();
      });
  }
}
let boardMatrix = [];

function renderBoard() {
  fetch("http://localhost:8080/board_state")
    .then((res) => res.json())
    .then((matrix) => {
      boardMatrix = matrix;
      drawBoard();
    });
}

function drawBoard() {
  boardEl.innerHTML = "";
  for (let y = 0; y < 8; y++) {
    for (let x = 0; x < 8; x++) {
      const square = createSquare(x, 7 - y); // reverse Y-axis
      const piece = boardMatrix[y][x];
      if (piece) square.textContent = pieceUnicode[piece] || piece;

      if (selected && selected.x === x + 1 && selected.y === 8 - y) {
        square.classList.add("selected");
      }
      if (validMoves.some((m) => m.x === x + 1 && m.y === 8 - y)) {
        square.classList.add("valid");
      }
      boardEl.appendChild(square);
    }
  }
}

renderBoard();
