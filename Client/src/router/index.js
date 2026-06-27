import { createRouter, createWebHashHistory } from 'vue-router'

import AuthLayout from '@/layout/AuthLayout.vue'
import MainLayout from '@/layout/MainLayout.vue'

import LoginView from '@/views/auth/Login.vue'
import RegisterView from '@/views/auth/Register.vue'
import ResetPasswordView from '@/views/auth/ResetPassword.vue'
import DashboardView from '@/views/dashboard/Dashboard.vue'
import VehicleList from '@/views/vehicle/VehicleList.vue'
import VehicleDetail from '@/views/vehicle/VehicleDetail.vue'
import VehicleForm from '@/views/vehicle/VehicleForm.vue'
import RentalList from '@/views/rental/RentalList.vue'
import RentalCreate from '@/views/rental/RentalCreate.vue'
import RentalDetail from '@/views/rental/RentalDetail.vue'
import MaintenanceList from '@/views/maintenance/MaintenanceList.vue'
import MaintenanceForm from '@/views/maintenance/MaintenanceForm.vue'
import Statistics from '@/views/stats/Statistics.vue'
import UserProfile from '@/views/user/UserProfile.vue'
import UserList from '@/views/user/UserList.vue'
import BalanceView from '@/views/user/BalanceView.vue'
import TopupView from '@/views/user/TopupView.vue'

const ROLE_MAP = { 0: 'customer', 1: 'staff', 2: 'admin' }

const routes = [
  {
    path: '/login',
    component: AuthLayout,
    meta: { guest: true },
    children: [
      { path: '', name: 'Login', component: LoginView },
    ],
  },
  {
    path: '/register',
    component: AuthLayout,
    meta: { guest: true },
    children: [
      { path: '', name: 'Register', component: RegisterView },
    ],
  },
  {
    path: '/reset-password',
    component: AuthLayout,
    meta: { guest: true },
    children: [
      { path: '', name: 'ResetPassword', component: ResetPasswordView },
    ],
  },
  {
    path: '/',
    component: MainLayout,
    meta: { requiresAuth: true },
    redirect: '/dashboard',
    children: [
      { path: 'dashboard', name: 'Dashboard', component: DashboardView, meta: { title: '工作台' } },
      { path: 'vehicles', name: 'VehicleList', component: VehicleList, meta: { title: '车辆管理' } },
      { path: 'vehicles/:id', name: 'VehicleDetail', component: VehicleDetail, meta: { title: '车辆详情' } },
      { path: 'vehicles/add', name: 'VehicleAdd', component: VehicleForm, meta: { title: '添加车辆', roles: ['admin'] } },
      { path: 'vehicles/:id/edit', name: 'VehicleEdit', component: VehicleForm, meta: { title: '编辑车辆', roles: ['admin'] } },
      { path: 'rentals', name: 'RentalList', component: RentalList, meta: { title: '租赁管理' } },
      { path: 'rentals/create', name: 'RentalCreate', component: RentalCreate, meta: { title: '新建租赁' } },
      { path: 'rentals/:id', name: 'RentalDetail', component: RentalDetail, meta: { title: '租赁详情' } },
      { path: 'maintenance', name: 'MaintenanceList', component: MaintenanceList, meta: { title: '维保管理', roles: ['staff', 'admin'] } },
      { path: 'maintenance/add', name: 'MaintenanceAdd', component: MaintenanceForm, meta: { title: '添加维保', roles: ['staff', 'admin'] } },
      { path: 'maintenance/:id/edit', name: 'MaintenanceEdit', component: MaintenanceForm, meta: { title: '编辑维保', roles: ['staff', 'admin'] } },
      { path: 'balance', name: 'BalanceView', component: BalanceView, meta: { title: '我的余额' } },
      { path: 'topup', name: 'TopupView', component: TopupView, meta: { title: '充值管理', roles: ['staff', 'admin'] } },
      { path: 'statistics', name: 'Statistics', component: Statistics, meta: { title: '统计报表', roles: ['admin'] } },
      { path: 'profile', name: 'UserProfile', component: UserProfile, meta: { title: '个人信息' } },
      { path: 'users', name: 'UserList', component: UserList, meta: { title: '用户管理', roles: ['admin'] } },
    ],
  },
]

const router = createRouter({
  history: createWebHashHistory(),
  routes,
})

router.beforeEach((to, from, next) => {
  const token = localStorage.getItem('token')
  const rawRole = localStorage.getItem('role') || '0'
  const roleKey = ROLE_MAP[rawRole] || 'user'

  if (to.matched.some((r) => r.meta.guest)) {
    if (token) return next('/dashboard')
    return next()
  }

  if (to.matched.some((r) => r.meta.requiresAuth)) {
    if (!token) return next('/login')
    const routeRoles = to.meta.roles
    if (routeRoles && !routeRoles.includes(roleKey)) return next('/dashboard')
  }

  next()
})

export default router
