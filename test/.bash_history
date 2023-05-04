git config --global user.name "kimhyojoon"
git config --global user.email "32gurihs@gmail.com"
mkdir test
cd test
touch 20210800.txt
git add 20210800.txt
git commit -m 'txt 파일 추가함'
vim 20210800.txt
git add 20210800.txt
git commit -m '이름적음'
git rm --cached 20210800.txt
git commit -m '삭제함'
git log
git status
