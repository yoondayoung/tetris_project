#include "tetris.h"

static struct sigaction act, oact;

int main() {
	int exit = 0;

	initscr();
	noecho();
	keypad(stdscr, TRUE);

	srand((unsigned int)time(NULL));

	createRankList();
	while (!exit) {
		clear();
		switch (menu()) {
		case MENU_PLAY: play(); break;
		case MENU_RANK: rank(); break;
		case MENU_EXIT: exit = 1; break;
		case MENU_RECOMMEND_PLAY: recommendedPlay(); break;
		default: break;
		}
	}

	writeRankFile();
	endwin();
	system("clear");
	return 0;
}

void InitTetris() {
	int i, j;

	for (j = 0; j < HEIGHT; j++)
		for (i = 0; i < WIDTH; i++)
			field[j][i] = 0;

	for (int i = 0; i < VISIBLE_BLOCKS; i++) {
		nextBlock[i] = rand() % 7;
	}
	blockRotate = 0;
	blockY = -1;
	blockX = WIDTH / 2 - 2;
	score = 0;
	gameOver = 0;
	timed_out = 0;
	//initialize tree
	recRoot_ = (RecNode_*)malloc(sizeof(RecNode_));
	recRoot_->lv = 0;
	recRoot_->accScore = 0;
	//end
	modified_recommend(recRoot_, field);

	DrawOutline();
	DrawField();
	DrawBlock(blockY, blockX, nextBlock[0], blockRotate, ' ');
	DrawNextBlock(nextBlock); // 1번째, 2번째 다음블록 그려주기
	PrintScore(score);
}

void DrawOutline() {
	int i, j;
	/* 블럭이 떨어지는 공간의 태두리를 그린다.*/
	DrawBox(0, 0, HEIGHT, WIDTH);

	/* next block을 보여주는 공간의 태두리를 그린다.*/
	move(2, WIDTH + 10);
	printw("NEXT BLOCK");
	DrawBox(3, WIDTH + 10, 4, 8);
	DrawBox(9, WIDTH + 10, 4, 8); // 2번째 다음블록 상자 그리기

	/* score를 보여주는 공간의 태두리를 그린다.*/
	move(15, WIDTH + 10);
	printw("SCORE");
	DrawBox(16, WIDTH + 10, 1, 8);
}

int GetCommand() {
	int command;
	command = wgetch(stdscr);
	switch (command) {
	case KEY_UP:
		break;
	case KEY_DOWN:
		break;
	case KEY_LEFT:
		break;
	case KEY_RIGHT:
		break;
		/* case ' ':    // space key
				// fall block
				break; */
	case 'q':
	case 'Q':
		command = QUIT;
		break;
	default:
		command = NOTHING;
		break;
	}
	return command;
}

int ProcessCommand(int command) {
	int ret = 1;
	int drawFlag = 0;
	switch (command) {
	case QUIT:
		ret = QUIT;
		break;
	case KEY_UP:
		if ((drawFlag = CheckToMove(field, nextBlock[0], (blockRotate + 1) % 4, blockY, blockX)))
			blockRotate = (blockRotate + 1) % 4;
		break;
	case KEY_DOWN:
		if ((drawFlag = CheckToMove(field, nextBlock[0], blockRotate, blockY + 1, blockX)))
			blockY++;
		break;
	case KEY_RIGHT:
		if ((drawFlag = CheckToMove(field, nextBlock[0], blockRotate, blockY, blockX + 1)))
			blockX++;
		break;
	case KEY_LEFT:
		if ((drawFlag = CheckToMove(field, nextBlock[0], blockRotate, blockY, blockX - 1)))
			blockX--;
		break;
	default:
		break;
	}
	if (drawFlag) DrawChange(field, command, nextBlock[0], blockRotate, blockY, blockX);
	return ret;
}

void DrawField() {
	int i, j;
	for (j = 0; j < HEIGHT; j++) {
		move(j + 1, 1);
		for (i = 0; i < WIDTH; i++) {
			if (field[j][i] == 1) {
				attron(A_REVERSE);
				printw(" ");
				attroff(A_REVERSE);
			}
			else printw(".");
		}
	}
}


void PrintScore(int score) {
	move(17, WIDTH + 11);
	printw("%8d", score);
}

void DrawNextBlock(int *nextBlock) {
	int i, j, bnum;
	int starty = 4;
	for (bnum = 1; bnum < 3; bnum++) {
		for (i = 0; i < 4; i++) {
			move(starty + i, WIDTH + 13);
			for (j = 0; j < 4; j++) {
				if (block[nextBlock[bnum]][0][i][j] == 1) {
					attron(A_REVERSE);
					printw(" ");
					attroff(A_REVERSE);
				}
				else printw(" ");
			}
		}
		starty += 6;
	}
}

void DrawBlock(int y, int x, int blockID, int blockRotate, char tile) {
	int i, j;
	for (i = 0; i < 4; i++)
		for (j = 0; j < 4; j++) {
			if (block[blockID][blockRotate][i][j] == 1 && i + y >= 0) {
				move(i + y + 1, j + x + 1);
				if (tile == '.') {
					printw("%c", tile);
				}
				else {
					attron(A_REVERSE);
					printw("%c", tile);
					attroff(A_REVERSE);
				}
			}
		}

	move(HEIGHT, WIDTH + 10);
}

void DrawBox(int y, int x, int height, int width) {
	int i, j;
	move(y, x);
	addch(ACS_ULCORNER);
	for (i = 0; i < width; i++)
		addch(ACS_HLINE);
	addch(ACS_URCORNER);
	for (j = 0; j < height; j++) {
		move(y + j + 1, x);
		addch(ACS_VLINE);
		move(y + j + 1, x + width + 1);
		addch(ACS_VLINE);
	}
	move(y + j + 1, x);
	addch(ACS_LLCORNER);
	for (i = 0; i < width; i++)
		addch(ACS_HLINE);
	addch(ACS_LRCORNER);
}

void play() {
	int command;
	clear();
	act.sa_handler = BlockDown;
	sigaction(SIGALRM, &act, &oact);
	InitTetris();
	do {
		if (timed_out == 0) {
			alarm(1);
			timed_out = 1;
		}

		command = GetCommand();
		if (ProcessCommand(command) == QUIT) {
			alarm(0);
			DrawBox(HEIGHT / 2 - 1, WIDTH / 2 - 5, 1, 10);
			move(HEIGHT / 2, WIDTH / 2 - 4);
			printw("Good-bye!!");
			refresh();
			getch();

			return;
		}
	} while (!gameOver);

	alarm(0);
	getch();
	DrawBox(HEIGHT / 2 - 1, WIDTH / 2 - 5, 1, 10);
	move(HEIGHT / 2, WIDTH / 2 - 4);
	printw("GameOver!!");
	refresh();
	getch();
	newRank(score);
}

char menu() {
	printw("1. play\n");
	printw("2. rank\n");
	printw("3. recommended play\n");
	printw("4. exit\n");
	return wgetch(stdscr);
}

/////////////////////////첫주차 실습에서 구현해야 할 함수/////////////////////////

int CheckToMove(char f[HEIGHT][WIDTH], int currentBlock, int blockRotate, int blockY, int blockX) {
	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			if (block[currentBlock][blockRotate][i][j] == 1) {
				if (0 > blockY + i || blockY + i >= HEIGHT) return 0;
				if (0 > blockX + j || blockX + j >= WIDTH) return 0;
				if (f[blockY + i][blockX + j] == 1) return 0;
			}
		}
	}
	return 1;
}

void DrawChange(char f[HEIGHT][WIDTH], int command, int currentBlock, int blockRotate, int blockY, int blockX) {
	//1. 이전 블록 정보를 찾는다. ProcessCommand의 switch문을 참조할 것
	//2. 이전 블록 정보를 지운다. DrawBlock함수 참조할 것.
	int sy; // 그림자위치
	switch (command) {
	case KEY_UP:
		DrawBlock(blockY, blockX, currentBlock, (blockRotate + 3) % 4, '.');
		sy = blockY;
		while (CheckToMove(f, currentBlock, (blockRotate + 3) % 4, sy + 1, blockX))
			sy++;
		DrawBlock(sy, blockX, currentBlock, (blockRotate + 3) % 4, '.');
		break;
	case KEY_DOWN:
		DrawBlock(blockY - 1, blockX, currentBlock, blockRotate, '.');
		break;
	case KEY_RIGHT:
		DrawBlock(blockY, blockX - 1, currentBlock, blockRotate, '.');
		sy = blockY;
		while (CheckToMove(f, currentBlock, blockRotate, sy + 1, blockX - 1))
			sy++;
		DrawBlock(sy, blockX - 1, currentBlock, blockRotate, '.');
		break;
	case KEY_LEFT:
		DrawBlock(blockY, blockX + 1, currentBlock, blockRotate, '.');
		sy = blockY;
		while (CheckToMove(f, currentBlock, blockRotate, sy + 1, blockX + 1))
			sy++;
		DrawBlock(sy, blockX + 1, currentBlock, blockRotate, '.');
		break;
	default: break;
	}
	//3. 새로운 블록 정보를 그린다.
	DrawBlockWithFeatures(blockY, blockX, currentBlock, blockRotate);
}

void BlockDown(int sig) {
	if (CheckToMove(field, nextBlock[0], blockRotate, blockY + 1, blockX)) {
		blockY++;
		DrawChange(field, KEY_DOWN, nextBlock[0], blockRotate, blockY, blockX);
	}
	else {
		score += AddBlockToField(field, nextBlock[0], blockRotate, blockY, blockX);
		if (blockY == -2) gameOver = 1;
		else {
			score += DeleteLine(field);
			for (int i = 0; i < VISIBLE_BLOCKS - 1; i++) {
				nextBlock[i] = nextBlock[i + 1];
			}
			nextBlock[VISIBLE_BLOCKS - 1] = rand() % 7;
			//initialize tree
			recRoot_ = (RecNode_*)malloc(sizeof(RecNode_));
			recRoot_->lv = 0;
			recRoot_->accScore = 0;
			//end
			modified_recommend(recRoot_, field);
			blockRotate = 0; blockY = -2; blockX = WIDTH / 2 - 2; // block 초기화
			DrawNextBlock(nextBlock);
			PrintScore(score);
		}
		DrawField();
	}
	timed_out = 0;
}

int AddBlockToField(char f[HEIGHT][WIDTH], int currentBlock, int blockRotate, int blockY, int blockX) {
	//Block이 추가된 영역의 필드값을 바꾼다.
	//필드에 닿은 면적*10 점수 리턴
	int touched = 0;
	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			if (block[currentBlock][blockRotate][i][j] == 1) {
				if (blockY + i == HEIGHT - 1) touched++;
				else if (f[blockY + i + 1][blockX + j] == 1) touched++;
				f[blockY + i][blockX + j] = 1;
			}
		}
	}
	return touched * 10;
}

int DeleteLine(char f[HEIGHT][WIDTH]) {
	//1. 필드를 탐색하여, 꽉 찬 구간이 있는지 탐색한다.
	//2. 꽉 찬 구간이 있으면 해당 구간을 지운다. 즉, 해당 구간으로 필드값을 한칸씩 내린다.
	int erased_cnt = 0;
	int full_flag;
	for (int i = 0; i < HEIGHT; i++) {
		int j;
		full_flag = 1;
		for (j = 0; j < WIDTH; j++) {
			if (f[i][j] == 0) {
				full_flag = 0;
				break;
			}
		}
		if (full_flag) {
			erased_cnt++;
			for (int k = 0; k < i; k++) {
				for (int h = 0; h < WIDTH; h++) {
					f[i - k][h] = f[i - k - 1][h];
				}
			}
			for (int k = 0; k < WIDTH; k++) {
				f[0][k] = 0;
			}
		}
	}
	return erased_cnt * erased_cnt * 100;
}

///////////////////////////////////////////////////////////////////////////

// 6주차 과제
void DrawShadow(int y, int x, int blockID, int blockRotate) {
	/***********************************************************
*      블록이 떨어질 위치를 미리 보여준다.
*      input   : (int) 그림자를 보여줄 블록의 왼쪽 상단모서리의 y 좌표
*                (int) 왼쪽 상단 모서리의 x 좌표
*                (int) 블록의 모양
*                (int) 블록의 회전 횟수
*      return  : none
***********************************************************/
	while (CheckToMove(field, blockID, blockRotate, y + 1, x))
		y++;
	DrawBlock(y, x, blockID, blockRotate, '/');
}

// 6주차 과제
void DrawBlockWithFeatures(int y, int x, int blockID, int blockRotate) {
	/***********************************************************
*      해당 좌표(y,x)에 원하는 모양의 블록을 그리고 그림자를 표시한다.
*      input   : (int) 그리고자 하는 박스의 왼쪽 상단모서리의 y 좌표
*                (int) 왼쪽 상단 모서리의 x 좌표
*                (int) 블록의 모양
*                (int) 블록의 회전 횟수
*      return  : none
***********************************************************/
	DrawBlock(y, x, blockID, blockRotate, ' ');
	DrawShadow(y, x, blockID, blockRotate);
	DrawRecommend(recommendY, recommendX, blockID, recommendR);
}


//------------------- 7주차 ----------------------------
void createRankList() {
	createq();
	int n, score; // 정보 개수, 점수
	char name[NAMELEN];
	FILE* fp = fopen("rank.txt", "r");

	if (fp == NULL) {
		fp = fopen("rank.txt", "w");
		fprintf(fp, "0\n");

	}

	else {
		fscanf(fp, "%d", &n);
		for (int i = 0; i < n; i++) {
			fscanf(fp, "%s%d", name, &score);
			addq(name, score);
		}
	}
	fclose(fp);
}

void rank() {
	int x = NULL;
	int y = NULL;
	int count = 0;
	int option;
	Node* current = front;
	char name[NAMELEN];
	int name_flag = false;
	int rank;

	while (option<'1' || option>'3') {
		clear();
		printw("1. list ranks from X to Y\n");
		printw("2. list ranks by a specific name\n");
		printw("3. delete a specific rank\n");
		option = wgetch(stdscr);
	}
	switch (option) {
	case '1':
		echo();
		printw("X: ");
		scanw("%d", &x);
		printw("Y: ");
		scanw("%d", &y);

		if (x == NULL) {
			x = 1;
		}
		if (y == NULL) {
			y = queue_size;
		}

		printw("      name      |   score   \n");
		printw("--------------------------------\n");

		//잘못된 조건인지 체크
		if (x > y || x < 1 || y < 1 || x>queue_size || y>queue_size)
			printw("search failure: no rank in the list");
		else {
			// 시작위치로 가주기
			for (int i = 1; i < x; i++) current = current->link;
			// 범위 벗어났거나 더 이상 탐색할 노드 없다면 기능 종료
			while (count < y - x + 1 && current) {
				printw("%-16s|  %d\n", current->name, current->score);
				current = current->link;
				count++;
			}
		}
		break;
	case '2':
		echo();
		printw("input the name: ");
		scanw("%s", name);
		printw("      name      |   score   \n");
		printw("--------------------------------\n");
		if (queue_size == 0); // queue가 empty일 때
		else {
			while (current) {
				if (strcmp(current->name, name) == 0) {
					name_flag = true;
					printw("%-16s|  %d\n", current->name, current->score);
				}
				current = current->link;
			}

		}
		if (name_flag == false)
			printw("search failure: no name in the list");
		break;
	case '3': // 삭제 기능
		echo();
		printw("input the rank: ");
		scanw("%d", &rank);
		if (queue_size == 0 || 1 > rank || rank > queue_size) // queue가 empty, count가 범위 벗어날 때
			printw("\nsearch failure: the rank not in the list");
		else if (rank == 1) { // 1위를 지워야 할 경우
			deleteq();
			printw("\nresult: the rank deleted");
		}
		else {
			count = 1; // count는 1부터 시작
			while (current) {
				if (count + 1 == rank) {
					deletepos(current);
					break;
				}
				current = current->link;
				count++;
			}
			printw("\nresult: the rank deleted");
		}
		break;
	}
	noecho();
	getch();
}

void writeRankFile() {
	FILE* fp = fopen("rank.txt", "w");
	Node* temp = front;

	fprintf(fp, "%d\n", queue_size);
	while (temp) {
		fprintf(fp, "%s %d\n", temp->name, temp->score);
		temp = temp->link;
	}
	fclose(fp);
}

void newRank(int score) {
	char name[NAMELEN];
	Node* temp;

	clear();
	echo();
	printw("your name: ");
	scanw("%s", name);

	if (queue_size == 0) // queue가 empty일 때
		addq(name, score);
	else {
		temp = front;
		if (score > temp->score) {
			addfirst(name, score); // 삽입 위치가 front일 때
		}
		else {
			while (temp->link && score <= temp->link->score) {
				temp = temp->link;
			}
			addpos(temp, name, score);
		}
	}


}

//7주차 실습-------------------------------------------

// =========== 8주차 실습 ==========================

void DrawRecommend(int y, int x, int blockID, int blockRotate) {
	DrawBlock(y, x, blockID, blockRotate, 'R');
}

int recommend(RecNode *root) {
	int temp_score, r;
	int max = -1; // 미리 보이는 블럭의 추천 배치까지 고려했을 때 얻을 수 있는 최대 점수
	RecNode *node = (RecNode*)malloc(sizeof(RecNode));
	node->lv = (root->lv) + 1; // 레벨 결정
	node->id = nextBlock[node->lv - 1]; // 블록id 결정
	switch (node->id) {
	case 0: r = 2;
	case 1: r = 4;
	case 2: r = 4;
	case 3: r = 4;
	case 4: r = 1;
	case 5: r = 2;
	case 6: r = 2;
	} // 회전수 구하기

	for (int i = 0; i < r; i++) {
		for (int j = -2; j < WIDTH; j++) { // 가능한 모든 x 위치
			node->y = 0;
			if (CheckToMove(root->recf, node->id, i, node->y, j)) {
				node->x = j; // x 위치 저장
				while (CheckToMove(root->recf, node->id, i, (node->y) + 1, node->x))
					(node->y)++; // y 위치 저장
				node->r = i; // 회전수 저장
				copy_field(root->recf, node->recf);
				node->accScore = root->accScore;
				node->accScore += AddBlockToField(node->recf, node->id, i, node->y, node->x);
				node->accScore += DeleteLine(node->recf); // 해당 블럭에서의 점수 구하기
				// 정보 저장
				if (node->lv < VISIBLE_BLOCKS) {
					//재귀적으로 다음 레벨의 노드 점수 구함
					temp_score = recommend(node);
				}
				else {
					// 마지막 레벨일 때 가장 큰 점수 구함
					temp_score = node->accScore;
				}
				if (max < temp_score) {
					max = temp_score;
					if (node->lv == 1) {
						recommendX = node->x;
						recommendY = node->y;
						recommendR = node->r;
					}
				}
			}
		}
	}

	return max;
}

int modified_recommend(RecNode_* root, char f[HEIGHT][WIDTH]) {
	int temp_score, r;
	int max = -1; // 미리 보이는 블럭의 추천 배치까지 고려했을 때 얻을 수 있는 최대 점수
	RecNode_ *node = (RecNode_*)malloc(sizeof(RecNode_));
	count++; // 노드 하나가 생성될 때마다 1 증가 -> 공간 측정
	node->lv = (root->lv) + 1; // 레벨 결정
	char buffer[HEIGHT][WIDTH];
	switch (nextBlock[node->lv - 1]) {
	case 0: r = 2;
	case 1: r = 4;
	case 2: r = 4;
	case 3: r = 4;
	case 4: r = 1;
	case 5: r = 2;
	case 6: r = 2;
	} // 회전수 구하기
	copy_field(f, buffer);

	for (int i = 0; i < r; i++) {
		for (int j = -2; j < WIDTH; j++) { // 가능한 모든 x 위치
			node->y = 0;
			copy_field(buffer, recf_);
			if (CheckToMove(recf_, nextBlock[node->lv - 1], i, node->y, j)) {
				while (CheckToMove(recf_, nextBlock[node->lv - 1], i, (node->y) + 1, j))
					(node->y)++; // y 위치 저장
				node->accScore = root->accScore;
				node->accScore += AddBlockToField(recf_, nextBlock[node->lv - 1], i, node->y, j);
				node->accScore += DeleteLine(recf_); // 해당 블럭에서의 점수 구하기
				if (node->accScore >= 1600) { // 만약 누적 점수가 1600점 이상이면
					if (node->lv == 1) {
						recommendX = j;
						recommendY = node->y;
						recommendR = i;
					}
					return node->accScore; // 그 노드는 더이상 함수 재귀호출 하지 않음
				} // pruning
				// 정보 저장
				if (node->lv < VISIBLE_BLOCKS) {
					//재귀적으로 다음 레벨의 노드 점수 구함
					temp_score = modified_recommend(node, recf_);
				}
				else {
					// 마지막 레벨일 때 가장 큰 점수 구함
					temp_score = node->accScore;
				}
				if (max < temp_score) {
					max = temp_score;
					if (node->lv == 1) {
						recommendX = j;
						recommendY = node->y;
						recommendR = i;
					}
					if (max >= 1600) return max; // pruning,  그 노드까지의 누적 점수가 1600점 이상이면 추천 시스템 종료
				}
			}
		}
	}

	return max;
}

void recommendedPlay() {
	int command;
	clear();
	act.sa_handler = RecommendBlockDown;
	sigaction(SIGALRM, &act, &oact);
	InitTetris();
	//시간 측정 시작
	start_t = time(NULL);
	do {
		if (timed_out == 0) {
			alarm(1);
			timed_out = 1;
		}

		command = GetCommand();
		if (command == QUIT) {
			stop_t = time(NULL); // 시간 측정 끝
			alarm(0);
			DrawBox(HEIGHT / 2 - 1, WIDTH / 2 - 5, 1, 10);
			move(HEIGHT / 2, WIDTH / 2 - 4);
			printw("Good-bye!!");
			refresh();
			getch();
			show_result(); // 측정 결과 출력
			return;
		}
	} while (!gameOver);
	//시간 측정 끝
	stop_t = time(NULL);
	alarm(0);
	getch();
	DrawBox(HEIGHT / 2 - 1, WIDTH / 2 - 5, 1, 10);
	move(HEIGHT / 2, WIDTH / 2 - 4);
	printw("GameOver!!");
	refresh();
	getch();
	show_result(); // 측정 결과 출력
}

void RecommendBlockDown(int sig) {
	if (CheckToMove(field, nextBlock[0], blockRotate, blockY + 1, blockX)) {
		RecommendDrawChange(field, KEY_DOWN, nextBlock[0], blockRotate, blockY, blockX);
		blockX = recommendX;
		blockY = recommendY;
		blockRotate = recommendR;
	}
	else {
		score += AddBlockToField(field, nextBlock[0], blockRotate, blockY, blockX);
		if (blockY == -1) gameOver = 1;
		else {
			score += DeleteLine(field);
			for (int i = 0; i < VISIBLE_BLOCKS - 1; i++) {
				nextBlock[i] = nextBlock[i + 1];
			}
			nextBlock[VISIBLE_BLOCKS - 1] = rand() % 7;
			start = time(NULL); // 시간 측정시작
			//initialize tree
			recRoot_ = (RecNode_*)malloc(sizeof(RecNode_));
			recRoot_->lv = 0;
			recRoot_->accScore = 0;
			//end
			modified_recommend(recRoot_, field);
			stop = time(NULL); // 시간 측정 종료
			blockRotate = 0; blockY = -1; blockX = WIDTH / 2 - 2; // block 초기화
			DrawNextBlock(nextBlock);
			PrintScore(score);
		}
		DrawField();
	}
	timed_out = 0;
	duration += (double)difftime(stop, start);
}

void RecommendDrawChange(char f[HEIGHT][WIDTH], int command, int currentBlock, int blockRotate, int blockY, int blockX) {
	//1. 이전 블록 정보를 찾는다. ProcessCommand의 switch문을 참조할 것
	//2. 이전 블록 정보를 지운다. DrawBlock함수 참조할 것.
	int sy; // 그림자위치
	switch (command) {
	case KEY_UP:
		DrawBlock(blockY, blockX, currentBlock, (blockRotate + 3) % 4, '.');
		break;
	case KEY_DOWN:
		DrawBlock(blockY - 1, blockX, currentBlock, blockRotate, '.');
		break;
	case KEY_RIGHT:
		DrawBlock(blockY, blockX - 1, currentBlock, blockRotate, '.');
		break;
	case KEY_LEFT:
		DrawBlock(blockY, blockX + 1, currentBlock, blockRotate, '.');
		break;
	default: break;
	}
	//3. 새로운 블록 정보를 그린다.
	DrawBlock(blockY, blockX, currentBlock, blockRotate, ' ');
	DrawRecommend(recommendY, recommendX, currentBlock, recommendR);
}

void show_result() {
	long space;
	duration_t = (double)difftime(stop_t, start_t);
	space = sizeof(RecNode_)*count;
	clear();
	printw("The total play time: is %lf seconds\n", duration_t);
	printw("The total score is: %d\n", score);
	printw("The total time for constructing tree is: %lf seconds\n", duration);
	printw("The total space for tree nodes is: %ld bytes\n", space);
	printw("score/time(seconds) : %lf\n", score / duration);
	printw("score/space(bytes) : %lf\n", score / (double)space);
	getch();
}