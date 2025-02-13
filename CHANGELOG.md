# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.3.0]
### Fixed
- 待修复：无间隔读取DHT11可能会出现各种通信错误的问题
- 待修复：长时间待机心跳机制出错，而导致断连的问题

## [1.2.0] - 2025-02-14
### Added
- 增加了通信质量检测，当通信质量极差时设备将重启以解决问题

## [1.1.0] - 2025-02-13
### Removed
- 删除了项目中作者用于测试给的信息

### Fixed
- 修复了无间隔进行TCP通信可能导致TCP断连的问题


## [1.0.0] - 2025-02-13
### Added
- 项目初始版本发布，包含四种交互方式、设备数据获取等基础功能
