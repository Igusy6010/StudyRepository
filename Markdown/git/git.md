# git コマンドメモ

## リポジトリの初期化

実行したフォルダパスに初期フォルダを作成
> git init

## ブランチ関連

ブランチの一覧を表示する。
選択中のブランチも確認することができるコマンド
> git branch

新たにブランチを作成する。
作成したブランチに切り替えはしない。
> git branch <NewBranchName>

指定したブランチに切り替える。
> git checkout <BranchName>

指定したリポジトリを取得する(クローンする)。
> git clone <RepositoryURL>

ローカルの変更状態を確認する。
> git status

## gitの確認関連

設定されている内容の一覧表示
> git config --list

ユーザーの変更(ユーザー名とメールアドレスの指定)
> git config --global user.name "<ユーザー名>"
>
> git config --global user.email <メールアドレス>
>
> ※Windows 場合は、資格情報の管理から github.com の情報を削除する必要があるかも?
>
> 　資格情報の管理画面は「Win」+「s」から『資格情報』と検索することで表示することができる。
